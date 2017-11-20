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

#include "Type.h"

#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "common.h"
#include "output.h"
#include "BasicType.h"
#include "CompositeType.h"
#include "EnumType.h"
#include "SetType.h"
#include "RefType.h"
#include "log.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{

void writeFileHeader(std::ostream& out, const std::string& name)
{
    out << "/// \\file\n"
           "/// \\brief Contains definition of \\ref " << common::fieldNamespaceStr() << name << " field.\n\n"
           "#pragma once\n\n";
}

void openNamespaces(std::ostream& out, DB& db)
{
    common::writeProtocolNamespaceBegin(db.getProtocolNamespace(), out);

    out << "namespace " << common::fieldNamespaceNameStr() << "\n"
            "{\n"
            "\n";
}

void closeNamespaces(std::ostream& out, DB& db)
{
    out << "} // namespace " << common::fieldNamespaceNameStr() << "\n"
            "\n";

    common::writeProtocolNamespaceEnd(db.getProtocolNamespace(), out);
}

} // namespace

Type::Type(DB& db, xmlNodePtr node)
  : m_db(db),
    m_node(node)
{
    m_props = xmlParseNodeProps(m_node, getDb().getDoc());
}

Type::~Type() noexcept = default;

bool Type::parse()
{
    if (m_props.empty()) {
        log::error() << "No properties for \"" << getNodeName() << "\" type." << std::endl;
        return false;
    }

    auto& name = getName();
    if (name.empty()) {
        log::error() << "No name has been specified for \"" << getNodeName() << "\" type." << std::endl;
        return false;
    }

    if ((!isRequired()) &&
        (!isOptional()) &&
        (!isConstant())) {
        log::error() << "Unknwon presence token \"" << prop::presence(m_props) << "\" for type \"" << getName() << "\"." << std::endl;
        return false;
    }

    m_extraIncludes.insert('\"' + common::fieldBaseFileName() + '\"');
    return parseImpl();
}

bool Type::doesExist()
{
    assert(!m_props.empty());
    return m_db.doesElementExist(prop::sinceVersion(m_props));
}

const char* Type::getNodeName() const
{
    return reinterpret_cast<const char*>(getNode()->name);
}

const std::string& Type::getName() const
{
    assert(!m_props.empty());
    return prop::name(m_props);
}

const std::string& Type::getReferenceName() const
{
    return common::renameKeyword(getName());
}

const std::string& Type::getDescription() const
{
    assert(!m_props.empty());
    return prop::description(m_props);
}

bool Type::isRequired() const
{
    assert(!m_props.empty());
    return prop::isRequired(m_props);
}

bool Type::isOptional() const
{
    assert(!m_props.empty());
    return prop::isOptional(m_props);
}

bool Type::isConstant() const
{
    assert(!m_props.empty());
    return prop::isConstant(m_props);
}

const std::string& Type::getPresence() const
{
    assert(!m_props.empty());
    return prop::presence(m_props);
}

unsigned Type::getLengthProp() const
{
    assert(!m_props.empty());
    return prop::length(m_props);
}

unsigned Type::getOffset() const
{
    assert(!m_props.empty());
    return prop::offset(m_props);
}

const std::string& Type::getMinValue() const
{
    assert(!m_props.empty());
    return prop::minValue(m_props);
}

const std::string& Type::getMaxValue() const
{
    assert(!m_props.empty());
    return prop::maxValue(m_props);
}

const std::string& Type::getNullValue() const
{
    assert(!m_props.empty());
    return prop::nullValue(m_props);
}

const std::string& Type::getSemanticType() const
{
    assert(!m_props.empty());
    return prop::semanticType(m_props);
}

const std::string& Type::getCharacterEncoding() const
{
    assert(!m_props.empty());
    return prop::characterEncoding(m_props);
}

const std::string& Type::getEncodingType() const
{
    assert(!m_props.empty());
    return prop::encodingType(m_props);
}

std::pair<std::string, bool> Type::getFailOnInvalid() const
{
    assert(!m_props.empty());
    auto& str = prop::ccFailInvalid(m_props);
    if (str.empty()) {
        return std::make_pair(str, false);
    }

    static const std::map<std::string, std::string> Map = {
        std::make_pair("default", std::string()),
        std::make_pair("data", "comms::ErrorStatus::InvalidMsgData"),
        std::make_pair("protocol", "comms::ErrorStatus::ProtocolError")
    };

    auto iter = Map.find(str);
    if (iter == Map.end()) {
        return std::make_pair(common::emptyString(), false);
    }

    return std::make_pair(iter->second, true);
}

void Type::updateExtraIncludes(ExtraIncludes& extraIncludes)
{
    for (auto& i : m_extraIncludes) {
        common::recordExtraHeader(i, extraIncludes);
    }
}

Type::Ptr Type::create(DB& db, xmlNodePtr node)
{
    std::string name(reinterpret_cast<const char*>(node->name));
    using TypeCreateFunc = std::function<Ptr (DB&, xmlNodePtr)>;
    static const std::map<std::string, TypeCreateFunc> Map = {
        std::make_pair(
            "type",
            [](DB& d, xmlNodePtr n)
            {
                return Ptr(new BasicType(d, n));
            }),
        std::make_pair(
            "composite",
            [](DB& d, xmlNodePtr n)
            {
                return Ptr(new CompositeType(d, n));
            }),
        std::make_pair(
            "enum",
            [](DB& d, xmlNodePtr n)
            {
                return Ptr(new EnumType(d, n));
            }),
        std::make_pair(
            "set",
            [](DB& d, xmlNodePtr n)
            {
                return Ptr(new SetType(d, n));
            }),
        std::make_pair(
            "ref",
            [](DB& d, xmlNodePtr n)
            {
                return Ptr(new RefType(d, n));
            })
    };

    auto createIter = Map.find(name);
    if (createIter == Map.end()) {
        std::cerr << "ERROR: Unknown type kind \"" << name << "\"." << std::endl;
        return Ptr();
    }

    return createIter->second(db, node);
}

bool Type::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace(), common::fieldDirName())) {
        return false;
    }

    auto fieldDirRelPath =
            common::protocolDirRelPath(m_db.getProtocolNamespace(), common::fieldDirName());

    const std::string Ext(".h");
    auto filename = getName() + Ext;
    auto relPath = bf::path(fieldDirRelPath) / filename;
    auto filePath = bf::path(m_db.getRootPath()) / relPath;

    log::info() << "Generating " << relPath.string() << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    writeFileHeader(out, getName());
    common::writeExtraHeaders(out, m_extraIncludes);
    openNamespaces(out, m_db);
    bool result = write(out);
    closeNamespaces(out, m_db);
    return result && out.good();
}

bool Type::write(std::ostream& out, unsigned indent)
{
    assert(doesExist());
    if (m_written) {
        return true;
    }

    if (m_writingInProgress) {
        log::error() << "Recursive types dependencies discovered for \"" << getName() << "\" type." << std::endl;
        return false;
    }

    m_writingInProgress = true;
    m_written = writeImpl(out, indent);
    m_writingInProgress = false;
    return m_written;
}

bool Type::parseImpl()
{
    return true;
}

bool Type::writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope)
{
    out << output::indent(indent) << "/// \\brief Default options for \\ref " << scope << getReferenceName() << " field.\n" <<
           output::indent(indent) << "using " << getReferenceName() << common::eqEmptyOptionStr() << ";\n\n";
    return true;
}

Type::ExtraOptInfosList Type::getExtraOptInfosImpl() const
{
    ExtraOptInfosList list;
    list.push_back(std::make_pair(getName(), getReferenceName()));
    return list;
}

bool Type::canBeExtendedAsOptionalImpl() const
{
    return false;
}

bool Type::writePluginPropertiesImpl(
        std::ostream& out,
        unsigned indent,
        const std::string& scope)
{
    std::string fieldType;
    std::string props;
    scopeToPropertyDefNames(scope, &fieldType, &props);
    out << output::indent(indent) << "using " << fieldType << " = " <<
           common::scopeFor(m_db.getProtocolNamespace(), common::fieldNamespaceStr() + scope + getName()) <<
           "<>;\n" <<
           output::indent(indent) << "auto " << props << " = \n" <<
           output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType << ">().name(";
    if (scope.empty()) {
        out  << common::fieldNameParamNameStr();
    }
    else {
        out << '\"' << getName() << '\"';
    }
    out << ");\n\n";

    if (scope.empty()) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }
    return true;
}

void Type::writeBrief(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "/// \\brief Definition of \"" << getName() << "\" field.\n";
}

void Type::writeHeader(std::ostream& out, unsigned indent, bool extraOpts)
{
    writeBrief(out, indent);
    common::writeDetails(out, indent, prop::description(m_props));
    if (extraOpts) {
        common::writeExtraOptionsDoc(out, indent);
    }
}

void Type::writeElementBrief(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "/// \\brief Element of \\ref " << getName() << " list field.\n";
}

void Type::writeElementHeader(std::ostream& out, unsigned indent)
{
    writeElementBrief(out, indent);
    common::writeExtraOptionsDoc(out, indent);
}

void Type::writeExtraOptions(std::ostream& out, unsigned indent)
{
    for (auto& o : m_extraOptions) {
        out << ",\n" <<
               output::indent(indent) << o;
    }
}

std::string Type::nodeText()
{
    return xmlText(m_node);
}

void Type::addExtraInclude(const std::string& val)
{
    common::recordExtraHeader(val, m_extraIncludes);
}

std::size_t Type::primitiveLength(const std::string& type)
{
    static const std::map<std::string, std::size_t> Map = {
        std::make_pair("char", sizeof(char)),
        std::make_pair("int8", sizeof(std::int8_t)),
        std::make_pair("uint8", sizeof(std::uint8_t)),
        std::make_pair("int16", sizeof(std::int16_t)),
        std::make_pair("uint16", sizeof(std::uint16_t)),
        std::make_pair("int32", sizeof(std::int32_t)),
        std::make_pair("uint32", sizeof(std::uint32_t)),
        std::make_pair("int64", sizeof(std::int64_t)),
        std::make_pair("uint64", sizeof(std::uint64_t)),
        std::make_pair("float", sizeof(float)),
        std::make_pair("double", sizeof(double)),
    };

    auto iter = Map.find(type);
    if (iter == Map.end()) {
        assert(!"Unexpected primitive");
        return 0U;
    }
    return iter->second;
}

std::pair<std::intmax_t, bool> Type::stringToInt(const std::string& str)
{
    try {
        return std::make_pair(std::stoll(str), true);
    }
    catch (...) {
        return std::make_pair(0, false);
    }
}

std::intmax_t Type::builtInIntNullValue(const std::string& type)
{
    static const std::map<std::string, std::intmax_t> Map = {
        std::make_pair(common::charType(), 0),
        std::make_pair("std::int8_t", common::intMinValue("int8", std::string()).first - 1),
        std::make_pair("std::uint8_t", common::intMaxValue("uint8", std::string()).first + 1),
        std::make_pair("std::int16_t", common::intMinValue("int16", std::string()).first - 1),
        std::make_pair("std::uint16_t", common::intMaxValue("uint16", std::string()).first + 1),
        std::make_pair("std::int32_t", common::intMinValue("int32", std::string()).first - 1),
        std::make_pair("std::uint32_t", common::intMaxValue("uint32", std::string()).first + 1),
        std::make_pair("std::int64_t", common::intMinValue("int64", std::string()).first - 1),
    };

    auto iter = Map.find(type);
    assert(iter != Map.end());
    return iter->second;
}

void Type::scopeToPropertyDefNames(const std::string& scope, std::string* fieldType, std::string* propsName)
{
    return scopeToPropertyDefNames(scope, getName(), fieldType, propsName);
}

void Type::scopeToPropertyDefNames(
    const std::string& scope,
    const std::string& name,
    std::string* fieldType,
    std::string* propsName)
{
    auto scopeNameStr = ba::replace_all_copy(scope, "::", "_");
    if (fieldType != nullptr) {
        *fieldType = "Field_" + scopeNameStr + name;
    }

    if (propsName != nullptr) {
        *propsName = "props_" + scopeNameStr + name;
    }

}


} // namespace sbe2comms
