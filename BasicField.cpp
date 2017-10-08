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
#include "BasicType.h"
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
        if (m_type->kind() == Type::Kind::Basic) {
            auto* basicType = static_cast<const BasicType*>(m_type);
            if (basicType->isFpType()) {
                recordExtraHeader("<cmath>");
                recordExtraHeader("<limits>");
            }
        }
        return checkOptional();
    }

    if (isConstant()) {
        return checkConstant();
    }

    log::error() << "Unknown presence token for field \"" << getName() << "\"." << std::endl;
    return false;
}

bool BasicField::writeImpl(std::ostream& out, DB& db, unsigned indent, const std::string& suffix)
{
    static_cast<void>(db);
    assert(m_type != nullptr);
    bool extraOpts = m_type->hasListOrString();
    writeBrief(out, indent, suffix, extraOpts);

    if (extraOpts) {
        writeOptions(out, indent);
    }

    auto name = getName() + suffix;

    if (isSimpleAlias()) {
        writeSimpleAlias(out, indent, name);
        return true;
    }

    if (isConstant()) {
        writeConstant(out, indent, name);
        return true;
    }

    if (isOptional()) {
        writeOptional(out, indent, name);
        return true;
    }

    log::error() << "The definition of \"" << getName() << "\" not implemented yet!" << std::endl;
    assert(!"Should not happen");
    return false;
}

bool BasicField::hasListOrStringImpl() const
{
    assert(m_type != nullptr);
    return (m_type != nullptr) && (m_type->hasListOrString());
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

    auto kind = m_type->kind();
    if ((kind != Type::Kind::Basic) &&
        (kind != Type::Kind::Enum)) {
        log::error() << "Optional field \"" << getName() << "\" can reference only "
                        "basic or enum type." << std::endl;
        return false;
    }

    if (m_type->hasListOrString()) {
        log::error() << "Optional field \"" << getName() << "\" can reference only "
                        "non-string or non-list types" << std::endl;
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

void BasicField::writeConstant(std::ostream& out, unsigned indent, const std::string& name)
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

void BasicField::writeOptional(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    if (m_type->kind() == Type::Kind::Basic) {
        writeOptionalBasic(out, indent, name);
        return;
    }

    assert(m_type->kind() == Type::Kind::Enum);
    writeOptionalEnum(out, indent, name);
}

void BasicField::writeOptionalBasic(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->kind() == Type::Kind::Basic);
    auto* basicType = static_cast<const BasicType*>(m_type);
    if (basicType->isIntType()) {
        writeOptionalBasicInt(out, indent, name);
        return;
    }

    assert(basicType->isFpType());
    writeOptionalBasicFp(out, indent, name);
}

void BasicField::writeOptionalBasicInt(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->kind() == Type::Kind::Basic);
    auto* basicType = static_cast<const BasicType*>(m_type);
    assert(basicType->isIntType());
    std::intmax_t nullValue = basicType->getDefultIntNullValue();
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n" <<
           output::indent(indent + 2) << "comms::option::DefaultNumValue<" << nullValue << ">,\n" <<
           output::indent(indent + 2) << "comms::option::ValidNumValue<" << nullValue << ">\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
           output::indent(indent + 1) << "bool isNull() const\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "using Base = typename std::decay<decltype(comms::field::toFieldBase(*this))>::type;\n" <<
           output::indent(indent + 2) << "return Base::value() == static_cast<Base::ValueType>(" << nullValue << ");\n" <<
           output::indent(indent + 1) << "}\n" <<
           output::indent(indent) << "};\n\n";
}

void BasicField::writeOptionalBasicFp(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->kind() == Type::Kind::Basic);
    auto* basicType = static_cast<const BasicType*>(m_type);
    assert(basicType->isFpType());
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "struct " << name << " : public " << fieldRefName << "<>\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Default constructor.\n" <<
           output::indent(indent + 1) << "/// \\details Initializes field to NaN.\n" <<
           output::indent(indent + 1) << name << "::" << name << "()\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "using Base = typename std::decay<decltype(comms::field::toFieldBase(*this))>::type;\n" <<
           output::indent(indent + 2) << "Base::value() = std::numeric_limits<typename Base::ValueType>::quiet_NaN();\n" <<
           output::indent(indent + 1) << "}\n\n" <<
           output::indent(indent + 1) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
           output::indent(indent + 1) << "bool isNull() const\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "using Base = typename std::decay<decltype(comms::field::toFieldBase(*this))>::type;\n" <<
           output::indent(indent + 2) << "return std::isnan(Base::value());\n" <<
           output::indent(indent + 1) << "}\n" <<
           output::indent(indent) << "};\n\n";
}

void BasicField::writeOptionalEnum(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->kind() == Type::Kind::Enum);
    auto* enumType = static_cast<const EnumType*>(m_type);
    std::intmax_t nullValue = enumType->getDefultNullValue();
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n" <<
           output::indent(indent + 2) << "comms::option::DefaultNumValue<" << nullValue << ">,\n" <<
           output::indent(indent + 2) << "comms::option::ValidNumValue<" << nullValue << ">\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
           output::indent(indent + 1) << "bool isNull() const\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "using Base = typename std::decay<decltype(comms::field::toFieldBase(*this))>::type;\n" <<
           output::indent(indent + 2) << "return Base::value() == static_cast<Base::ValueType>(" << nullValue << ");\n" <<
           output::indent(indent + 1) << "}\n" <<
           output::indent(indent) << "};\n\n";
}

} // namespace sbe2comms
