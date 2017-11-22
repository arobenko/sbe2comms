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
#include "BasicType.h"
#include "EnumType.h"
#include "CompositeType.h"
#include "common.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{

const std::string& getNamespaceForType(const DB& db, const std::string& name)
{
    if (db.isIntroducedType(name)) {
        return common::fieldNamespaceStr();
    }

    assert ((db.isRecordedBuiltInType(name)) || db.isRecordedPaddingType(name));
    return common::builtinNamespaceStr();
}

} // namespace

const std::string& BasicField::getValueRef() const
{
    auto& p = getProps();
    assert(!p.empty());
    return prop::valueRef(p);
}

unsigned BasicField::getSerializationLength() const
{
    assert(m_type != nullptr);
    return m_type->getSerializationLength();
}

Field::Kind BasicField::getKindImpl() const
{
    return Kind::Basic;
}

bool BasicField::parseImpl()
{
    auto getTypeHeader =
        [this](const std::string& fieldNs, const std::string& typeName) -> std::string
        {
            return
                '\"' +
                common::pathTo(
                    getDb().getProtocolNamespace(),
                    fieldNs + '/' + typeName + ".h") +
                '\"';
        };
    do {
        auto& typeName = getType();
        if (typeName.empty()) {
            if (isConstant()) {
                auto& valueRef = getValueRef();
                if (!valueRef.empty()) {
                    assert(!m_generatedPadding);
                    m_type = getTypeFromValueRef();
                    if (m_type != nullptr) {
                        recordExtraHeader(
                            getTypeHeader(common::fieldNamespaceNameStr(), m_type->getName()));
                    }
                    break;
                }
            }

            log::error() << "The field \"" << getName() << "\" doesn't specify its type." << std::endl;
            return false;
        }

        if (m_generatedPadding) {
            m_type = getDb().findPaddingType(typeName);
            assert(m_type != nullptr);
            recordExtraHeader(
                getTypeHeader(common::builtinNamespaceNameStr(), common::padStr()));
            break;
        }

        m_type = getDb().findType(typeName);
        if (m_type != nullptr) {
            recordExtraHeader(
                getTypeHeader(common::fieldNamespaceNameStr(), m_type->getName()));
            break;
        }

        m_type = getDb().getBuiltInType(typeName);
        assert((m_type == nullptr) || getDb().isRecordedBuiltInType(typeName));
        recordExtraHeader(
            getTypeHeader(common::fieldNamespaceNameStr(), common::fieldBaseStr()));
        recordExtraHeader(
            getTypeHeader(common::builtinNamespaceNameStr(), m_type->getName()));
    } while (false);

    if (m_type == nullptr) {
        log::error() << "Unknown or invalid type for field \"" << getName() << "\"." << std::endl;
        return false;
    }

    if (!m_type->hasFixedLength()) {
        log::error() << "Field \"" << getName() << "\" references type \"" << m_type->getName() <<
                        "\", which doesn't have fixed length." << std::endl;
        return false;
    }

    if ((m_type->getKind() == Type::Kind::Composite) &&
        (asCompositeType(m_type)->dataUseRecorded())) {
        log::error() << "Cannot use \"" << m_type->getName() << "\" type with \"" << getName() << "\" field due to "
                        "the former being referenced by \"data\" element(s)." << std::endl;
        return false;
    }

    if (!hasPresence()) {
        return true;
    }

    if (isRequired()) {
        return checkRequired();
    }

    if (isOptional()) {
        if (m_type->getKind() == Type::Kind::Basic) {
            if (asBasicType(m_type)->isFpType()) {
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

bool BasicField::writeImpl(std::ostream& out, unsigned indent, const std::string& suffix)
{
    assert(m_type != nullptr);
    writeHeader(out, indent, suffix);
    std::string name;
    if (suffix.empty()) {
        name = getReferenceName();
    }
    else {
        name = getName() + suffix;
    }

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

bool BasicField::usesBuiltInTypeImpl() const
{
    assert(m_type != nullptr);
    return m_generatedPadding || getDb().isRecordedBuiltInType(m_type->getName());
}

bool BasicField::writePluginPropertiesImpl(std::ostream& out,
    unsigned indent,
    const std::string& scope,
    bool returnResult,
    bool commsOptional)
{
    assert(m_type != nullptr);
    if (m_generatedPadding || getDb().isRecordedBuiltInType(m_type->getName())) {
        writeBuiltinPluginProperties(out, indent, scope, returnResult);
        return true;
    }

    if (isSimpleAlias()) {
        writeSimpleAliasPluginProperties(out, indent, scope, returnResult, commsOptional);
        return true;
    }

//    log::error() << "NYI: " << getName() << std::endl;
//    assert(0);
//    return false;

    out << output::indent(indent) << "// TODO: " << getName() << "\n";
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

    if (!m_type->canBeExtendedAsOptional()) {
        log::error() << "Type \"" << m_type->getName() << "\" cannot be extended as optional, please put \"presence=optional\" in type definition." << std::endl;
        return false;
    }

    auto kind = m_type->getKind();
    if ((kind != Type::Kind::Basic) &&
        (kind != Type::Kind::Enum)) {
        log::error() << "Optional field \"" << getName() << "\" can reference only "
                        "basic or enum type." << std::endl;
        return false;
    }

    return true;
}

bool BasicField::checkConstant() const
{
    auto& valueRef = getValueRef();
    assert(m_type != nullptr);
    if (m_type->getKind() == Type::Kind::Composite) {
        log::error() << "The field \"" << getName() << "\" references composite type \"" <<
                        m_type->getName() << "\". It cannot have constant presence." << std::endl;
        return false;
    }

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

    if (enumType->getKind() != Type::Kind::Enum) {
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

    if (type->getKind() != Type::Kind::Enum) {
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


void BasicField::writePaddingAlias(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    auto& ns = getNamespaceForType(getDb(), m_type->getName());

    out << output::indent(indent) << "using " << name << " = " << ns << common::padStr() << "<\n" <<
           output::indent(indent + 1) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 1) << common::num(m_type->getSerializationLength()) << ",\n" <<
           output::indent(indent + 1) << getFieldOptString() << '\n' <<
           output::indent(indent) << ">;\n\n";
    return;

}

void BasicField::writeCompositeAlias(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    auto& ns = getNamespaceForType(getDb(), m_type->getName());
    out << output::indent(indent) << "using " << name << " = " << ns << m_type->getReferenceName() << "<\n";
    auto typeOpts = m_type->getExtraOptInfos();
    for (auto& o : typeOpts) {
        out << output::indent(indent + 1) << common::optParamPrefixStr();
        if (!ba::starts_with(o.second, common::fieldNamespaceStr())) {
            out << common::fieldNamespaceStr();
        }
        out << o.second;
        bool comma = (&o != &typeOpts.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }

    out << output::indent(indent) << ">;\n\n";

}

void BasicField::writeSimpleAlias(std::ostream& out, unsigned indent, const std::string& name)
{
    if (m_generatedPadding) {
        writePaddingAlias(out, indent, name);
        return;
    }

    assert(m_type != nullptr);
    if (m_type->getKind() == Type::Kind::Composite) {
        writeCompositeAlias(out, indent, name);
        return;
    }

    auto& ns = getNamespaceForType(getDb(), m_type->getName());

    out << output::indent(indent) << "using " << name << " = " << ns << m_type->getReferenceName() << "<\n";

    bool builtIn = getDb().isRecordedBuiltInType(m_type->getName());
    if (builtIn) {
        out << output::indent(indent + 1) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n" <<
               output::indent(indent + 1) << getFieldOptString() << '\n' <<
               output::indent(indent) << ">;\n\n";
        return;
    }

    out << output::indent(indent + 1) << getTypeOptString(*m_type) << ",\n" <<
           output::indent(indent + 1) << getFieldOptString() << '\n' <<
           output::indent(indent) << ">;\n\n";
}

void BasicField::writeConstant(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    auto& valueRef = getValueRef();
    auto sep = ba::find_first(valueRef, ".");
    assert(sep);
    std::string enumType(valueRef.begin(), sep.begin());
    enumType += common::enumValSuffixStr();
    std::string valueStr(sep.end(), valueRef.end());
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    bool builtIn =
            (!getDb().isIntroducedType(m_type->getName())) &&
            (getDb().isRecordedBuiltInType(m_type->getName()));
    out << output::indent(indent) << "using " << name << " =\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n";
    if (builtIn) {
        out << output::indent(indent + 2) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n";
    }
    else {
        out << output::indent(indent + 2) << getTypeOptString(*m_type) << ",\n";
    }
    out << output::indent(indent + 2) << getFieldOptString() << ",\n" <<
           output::indent(indent + 2) << "comms::option::DefaultNumValue<(std::intmax_t)" << common::fieldNamespaceStr() << enumType << "::" << valueStr << ">,\n" <<
           output::indent(indent + 2) << "comms::option::EmptySerialization\n" <<
           output::indent(indent + 1) << ">;\n\n";
}

void BasicField::writeOptional(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    if (m_type->getKind() == Type::Kind::Basic) {
        writeOptionalBasic(out, indent, name);
        return;
    }

    assert(m_type->getKind() == Type::Kind::Enum);
    writeOptionalEnum(out, indent, name);
}

void BasicField::writeOptionalBasic(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->getKind() == Type::Kind::Basic);
    auto* basicType = static_cast<const BasicType*>(m_type);
    if (basicType->isIntType()) {
        writeOptionalBasicInt(out, indent, name);
        return;
    }

    assert(basicType->isFpType());
    writeOptionalBasicFp(out, indent, name);
}

void BasicField::writeOptionalBasicBigUnsignedInt(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->getKind() == Type::Kind::Basic);
    auto* basicType = static_cast<const BasicType*>(m_type);
    assert(basicType->isIntType());
    std::uintmax_t nullValue = common::defaultBigUnsignedNullValue();
    auto nullValueStr = common::num(nullValue);
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n";
    bool builtIn = getDb().isRecordedBuiltInType(m_type->getName());
    if (builtIn) {
        out << output::indent(indent + 2) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n";
    }
    else {
        out << output::indent(indent + 2) << getTypeOptString(*m_type) << ",\n";
    }

    out << output::indent(indent + 2) << getFieldOptString() << ",\n" <<
           output::indent(indent + 2) << "comms::option::DefaultBigUnsignedNumValue<" << nullValueStr << ">,\n" <<
           output::indent(indent + 2) << "comms::option::ValidBigUnsignedNumValue<" << nullValueStr << ">\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    common::writeIntNullCheckUpdateFuncs(out, indent + 1, nullValueStr);
    out << output::indent(indent) << "};\n\n";
}

void BasicField::writeOptionalBasicInt(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->getKind() == Type::Kind::Basic);
    auto* basicType = static_cast<const BasicType*>(m_type);
    assert(basicType->isIntType());
    if (basicType->getPrimitiveType() == common::uint64Type()) {
        writeOptionalBasicBigUnsignedInt(out, indent, name);
        return;
    }
    auto nullValueStr = common::num(basicType->getDefultIntNullValue());
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n";
    bool builtIn = getDb().isRecordedBuiltInType(m_type->getName());
    if (builtIn) {
        out << output::indent(indent + 2) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n";
    }
    else {
        out << output::indent(indent + 2) << getTypeOptString(*m_type) << ",\n";
    }

    out << output::indent(indent + 2) << getFieldOptString() << ",\n" <<
           output::indent(indent + 2) << "comms::option::DefaultNumValue<" << nullValueStr << ">,\n" <<
           output::indent(indent + 2) << "comms::option::ValidNumValue<" << nullValueStr << ">\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    common::writeIntNullCheckUpdateFuncs(out, indent + 1, nullValueStr);
    out << output::indent(indent) << "};\n\n";
}

void BasicField::writeOptionalBasicFp(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->getKind() == Type::Kind::Basic);
    auto* basicType = static_cast<const BasicType*>(m_type);
    assert(basicType->isFpType());
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n";
    bool builtIn = getDb().isRecordedBuiltInType(m_type->getName());
    if (builtIn) {
        out << output::indent(indent + 2) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n";
    }
    else {
        out << output::indent(indent + 2) << getTypeOptString(*m_type) << ",\n";
    }

    out << output::indent(indent + 2) << getFieldOptString() << '\n' <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    common::writeFpOptConstructor(out, indent + 1, name);
    out << '\n';
    common::writeFpNullCheckUpdateFuncs(out, indent + 1);
    out << '\n';
    common::writeFpValidCheckFunc(out, indent + 1, true);
    out << output::indent(indent) << "};\n\n";
}

void BasicField::writeOptionalEnum(std::ostream& out, unsigned indent, const std::string& name)
{
    assert(m_type != nullptr);
    assert(m_type->getKind() == Type::Kind::Enum);
    auto* enumType = static_cast<const EnumType*>(m_type);
    std::intmax_t nullValue = enumType->getDefultNullValue();
    auto nullValueStr = common::num(nullValue);
    auto fieldRefName = getNamespaceForType(getDb(), m_type->getName()) + m_type->getReferenceName();
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << fieldRefName << "<\n" <<
           output::indent(indent + 2) << getTypeOptString(*m_type) << ",\n" <<
           output::indent(indent + 2) << getFieldOptString() << ",\n" <<
           output::indent(indent + 2) << "comms::option::DefaultNumValue<" << nullValueStr << ">,\n" <<
           output::indent(indent + 2) << "comms::option::ValidNumValue<" << nullValueStr << ">\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    common::writeIntNullCheckUpdateFuncs(out, indent + 1, nullValueStr);
    out << output::indent(indent) << "};\n\n";
}

void BasicField::writeBuiltinPluginProperties(
        std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool returnResult)
{
    std::string fieldType;
    std::string props;
    scopeToPropertyDefNames(scope, &fieldType, &props);
    out << output::indent(indent) << "using " << fieldType << " = " << scope + getName() << ";\n" <<
           output::indent(indent) << "auto " << props << " = \n" <<
           output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType <<
           ">().name(" << '\"' << getName() << "\");\n\n";

    if (returnResult) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }
}

void BasicField::writeSimpleAliasPluginProperties(std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool returnResult,
        bool commsOptional)
{
    static_cast<void>(commsOptional);
    std::string props;
    scopeToPropertyDefNames(scope, nullptr, &props);

    out << output::indent(indent) << "auto " << props << " =\n" <<
           output::indent(indent + 1) << common::pluginNamespaceStr() <<
           common::fieldNamespaceStr() << "createProps_" << m_type->getName() <<
           "(\"" << getName() << "\");\n\n";

    if (returnResult) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }
}


} // namespace sbe2comms
