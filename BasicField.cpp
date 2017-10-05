//
// Copyright 2017 (C). Alex Robenko. All rights reserved.
//

// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "BasicField.h"

#include <iostream>
#include <set>

#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"
#include "get.h"
#include "EnumType.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{

const std::string FieldSuffix("Field");
const std::string ValSuffix("Val");
const std::string FieldNamespace("field::");
const std::string BuiltInNamespace("sbe2comms::");

const std::string& getNamespaceForType(const DB& db, const std::string& name)
{
    if (db.isRecordedBuiltInType(name)) {
        return BuiltInNamespace;
    }
    return FieldNamespace;
}

} // namespace

const std::string& BasicField::getType() const
{
    auto& p = getProps();
    assert(!p.empty());
    return prop::type(p);
}

const std::string& BasicField::getValueRef() const
{
    auto& p = getProps();
    assert(!p.empty());
    return prop::valueRef(p);
}

bool BasicField::parseImpl()
{
    do {
        auto& typeName = getType();
        if (typeName.empty()) {
            if (isConstant()) {
                auto& valueRef = getValueRef();
                if (!valueRef.empty()) {
                    m_type = getTypeFromValueRef();
                    break;
                }
            }

            log::error() << "The field \"" << getName() << "\" doesn't specify its type." << std::endl;
            return false;
        }

        m_type = getDb().findType(typeName);
        if (m_type != nullptr) {
            break;
        }

        m_type = getDb().getBuiltInType(typeName);
    } while (false);

    if (m_type == nullptr) {
        log::error() << "Unknown or invalid type for field \"" << getName() << "\"." << std::endl;
        return false;
    }

    if (!hasPresence()) {
        return true;
    }

    if (isRequired()) {
        return checkRequired();
    }

    if (isOptional()) {
        return checkOptional();
    }

    if (isConstant()) {
        return checkConstant();
    }

    log::error() << "Unknown presence token for field \"" << getName() << "\"." << std::endl;
    return false;
}

bool BasicField::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    static_cast<void>(db);
    assert(m_type != nullptr);

    auto& optMode = getDefaultOptMode();
    if (optMode.empty()) {
        writeFieldDef(out, indent, false);
        return true;
    }

    log::error() << "The optional wrapper field " << getName() << " write is not supported yet!" << std::endl;
    return true;

    writeFieldDef(out, indent, true);
    // TODO: write optional wrapper
    return true;
}

bool BasicField::checkRequired() const
{
    assert(m_type != nullptr);
    if (!m_type->isRequired()) {
        log::error() << "Required field \"" << getName() << "\" references "
                        "optional/constant type " << m_type->getName() << "\"." << std::endl;
        return false;
    }

    return true;
}

bool BasicField::checkOptional() const
{
    assert(m_type != nullptr);
    if (m_type->isConstant()) {
        log::error() << "Optional field \"" << getName() << "\" references "
                        "constant type " << m_type->getName() << "\"." << std::endl;
        return false;
    }

    return true;
}

bool BasicField::checkConstant() const
{
    auto& valueRef = getValueRef();
    assert(m_type != nullptr);
    if (m_type->isConstant()) {
        if (!valueRef.empty()) {
            log::error() << "The constant field \"" << getName() << "\" references constant type while providing valueRef." << std::endl;
            return false;
        }
        return true;
    }

    if (m_type->isOptional()) {
        log::error() << "Referencing optional type in constant \"" << getName() << "\" field is not supported." << std::endl;
        return false;
    }

    if (valueRef.empty()) {
        log::error() << "The constant field \"" << getName() << "\" must specify valueRef property." << std::endl;
        return false;
    }

    auto sep = ba::find_first(valueRef, ".");
    if (!sep) {
        log::error() << "Failed to split valueRef of \"" << getName() << "\" into <type.value> pair." << std::endl;
        return false;
    }

    std::string enumTypeStr(valueRef.begin(), sep.begin());
    if (enumTypeStr.empty()) {
        log::error() << "valueRef property of \"" << getName() << "\" field does not provide enum name." << std::endl;
        return false;
    }

    auto* enumType = getDb().findType(enumTypeStr);
    if (enumType == nullptr) {
        log::error() << "Enum type \"" << enumTypeStr << "\" referenced by \"" << getName() << "\" field is not defined." << std::endl;
        return false;
    }

    if (enumType->kind() != Type::Kind::Enum) {
        log::error() << "valueRef property of constant field \"" << getName() << "\" must specify enum type." << std::endl;
        return false;
    }

    std::string valueStr(sep.end(), valueRef.end());
    if (!static_cast<const EnumType*>(enumType)->hasValue(valueStr)) {
        log::error() << "The field \"" << getName() << "\" references invalid value \"" << valueRef << "\"." << std::endl;
        return false;
    }
    return true;
}

const Type* BasicField::getTypeFromValueRef() const
{
    auto& valueRef = getValueRef();
    assert(!valueRef.empty());

    auto sep = ba::find_first(valueRef, ".");
    if (!sep) {
        log::error() << "Failed to split valueRef into <type.value> pair." << std::endl;
        return nullptr;
    }

    std::string enumName(valueRef.begin(), sep.begin());
    auto* type = getDb().findType(enumName);
    if (type == nullptr) {
        log::error() << "Enum type \"" << enumName << "\" referenced by field \"" << getName() << "\" does not exist." << std::endl;
        return nullptr;
    }

    if (type->kind() != Type::Kind::Enum) {
        log::error() << "Type \"" << enumName << "\" rerences by field \"" << getName() << "\" is not an enum." << std::endl;
        return nullptr;
    }
    return type;
}

const std::string& BasicField::getDefaultOptMode()
{
    if (getDeprecated() <= getDb().getSchemaVersion()) {
        static const std::string Mode("comms::field::OptionalMode::Missing");
        return Mode;
    }

    if (getDb().getMinRemoteVersion() < getSinceVersion()) {
        static const std::string Mode("comms::field::OptionalMode::Exists");
        return Mode;
    }

    return get::emptyString();
}

bool BasicField::isSimpleAlias() const
{
    if ((!hasPresence()) || isRequired()) {
        return true;
    }

    if (isOptional() && m_type->isOptional()) {
        return true;
    }

    if (isConstant() && m_type->isConstant() && getValueRef().empty()) {
        return true;
    }

    return false;
}

void BasicField::writeSimpleAlias(std::ostream& out, unsigned indent, const std::string& name)
{
    bool builtIn = getDb().isRecordedBuiltInType(m_type->getName());
    auto& ns = getNamespaceForType(getDb(), m_type->getName());
    bool extraOpts = m_type->hasListOrString();
    out << output::indent(indent) << "using " << name << " = " << ns << m_type->getReferenceName() << "<";
    if (builtIn) {
        out << FieldNamespace << "FieldBase";
    }

    if (extraOpts) {
        if (builtIn) {
            out << ", ";
        }
        out << "TOpt...";
    }

    out << ">;\n\n";
}

void BasicField::writeConstantEnum(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    auto& valueRef = getValueRef();
    auto sep = ba::find_first(valueRef, ".");
    assert(sep);
    std::string enumType(valueRef.begin(), sep.begin());
    enumType += ValSuffix;
    std::string valueStr(sep.end(), valueRef.end());
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "using " << name << " =\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n" <<
           output::indent(indent + 2) << "comms::option::DefaultNumValue<(std::intmax_t)" << FieldNamespace << enumType << "::" << valueStr << ">,\n" <<
           output::indent(indent + 2) << "comms::option::EmptySerialization\n" <<
           output::indent(indent + 1) << ">;\n\n";
}

void BasicField::writeFieldDef(std::ostream& out, unsigned indent, bool wrapped)
{
    assert(m_type != nullptr);
    bool extraOpts = m_type->hasListOrString();
    if (wrapped) {
        writeWrappedFieldBrief(out, indent, extraOpts);
    }
    else {
        writeBrief(out, indent, extraOpts);
    }

    if (extraOpts) {
        writeOptions(out, indent);
    }

    auto name = getName();
    if (wrapped) {
        name += FieldSuffix;
    }

    if (isSimpleAlias()) {
        writeSimpleAlias(out, indent, name);
        return;
    }

    if (isConstant()) {
        writeConstantEnum(out, indent, name);
        return;
    }

    // TODO:
    log::error() << "The definition of \"" << getName() << "\" not implemented yet!" << std::endl;
}

void BasicField::writeWrappedFieldBrief(std::ostream& out, unsigned indent, bool extraOpts)
{
    auto& name = getName();
    assert(!name.empty());

    out << output::indent(indent) << "/// \\brief Definition of inner field of the optional \\ref " << name << " field.\n";
    auto& desc = getDescription();
    if (!desc.empty()) {
        out << output::indent(indent) << "/// \\details " << desc << '\n';
    }

    if (extraOpts) {
        out << output::indent(indent) << "/// \\tparam TOpt Extra options from \\b comms::option namespace.\n";
    }
}


} // namespace sbe2comms
