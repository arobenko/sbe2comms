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

#include "DB.h"
#include "common.h"
#include "output.h"
#include "BasicType.h"
#include "CompositeType.h"
#include "EnumType.h"
#include "SetType.h"
#include "RefType.h"
#include "log.h"

namespace sbe2comms
{

Type::Type(DB& db, xmlNodePtr node)
  : m_db(db),
    m_node(node)
{
    std::fill(m_uses.begin(), m_uses.end(), 0U);
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

    return parseImpl();
}

bool Type::doesExist()
{
    assert(!m_props.empty());
    return m_db.doesElementExist(prop::sinceVersion(m_props), prop::deprecated(m_props));
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
        auto iter = extraIncludes.lower_bound(i);
        if ((iter != extraIncludes.end()) &&
            (*iter == i)) {
            continue;
        }

        extraIncludes.insert(iter, i);
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

bool Type::writeDependenciesImpl(std::ostream& out, unsigned indent)
{
    static_cast<void>(out);
    static_cast<void>(indent);
    return true;
}

bool Type::hasListOrStringImpl() const
{
    return false;
}

Type::ExtraOptInfosList Type::getExtraOptInfosImpl() const
{
    ExtraOptInfosList list;
    list.push_back(std::make_pair(getName(), getReferenceName()));
    return list;
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

void Type::writeBaseDef(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "using Base = typename std::decay<decltype(comms::field::toFieldBase(*this))>::type;\n";
}

void Type::writeFailOnInvalid(std::ostream& out, unsigned indent)
{
    auto result = getFailOnInvalid();
    if (!result.second) {
        return;
    }

    out << ",\n" <<
           output::indent(indent) << "comms::option::FailOnInvalid<" << result.first << ">";
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
    };

    auto iter = Map.find(type);
    if (iter == Map.end()) {
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

const std::string& Type::primitiveTypeToStdInt(const std::string& type)
{
    static const std::map<std::string, std::string> Map = {
        std::make_pair("char", "char"),
        std::make_pair("int8", "std::int8_t"),
        std::make_pair("uint8", "std::uint8_t"),
        std::make_pair("int16", "std::int16_t"),
        std::make_pair("uint16", "std::uint16_t"),
        std::make_pair("int32", "std::int32_t"),
        std::make_pair("uint32", "std::uint32_t"),
        std::make_pair("int64", "std::int64_t"),
        std::make_pair("uint64", "std::uint64_t")
    };

    auto iter = Map.find(type);
    if (iter == Map.end()) {
        return common::emptyString();
    }

    return iter->second;
}

std::pair<std::intmax_t, bool> Type::intMinValue(const std::string& type, const std::string& value)
{
    if (value.empty()) {
        static const std::map<std::string, std::intmax_t> Map = {
            std::make_pair("char", 0x20),
            std::make_pair("int8", std::numeric_limits<std::int8_t>::min() + 1),
            std::make_pair("uint8", 0),
            std::make_pair("int16", std::numeric_limits<std::int16_t>::min() + 1),
            std::make_pair("uint16", 0),
            std::make_pair("int32", std::numeric_limits<std::int32_t>::min() + 1),
            std::make_pair("uint32", 0),
            std::make_pair("int64", std::numeric_limits<std::int64_t>::min() + 1),
            std::make_pair("uint64", 0)
        };

        auto iter = Map.find(type);
        assert(iter != Map.end());
        return std::make_pair(iter->second, true);
    }

    try {
        return std::make_pair(std::stoll(value), true);
    } catch(...) {
        return std::make_pair(std::intmax_t(0), false);
    }
}

std::pair<std::intmax_t, bool> Type::intMaxValue(const std::string& type, const std::string& value)
{
    if (value.empty()) {
        static const std::map<std::string, std::intmax_t> Map = {
            std::make_pair("char", 0x7e),
            std::make_pair("int8", std::numeric_limits<std::int8_t>::max()),
            std::make_pair("uint8", std::numeric_limits<std::uint8_t>::max() - 1),
            std::make_pair("int16", std::numeric_limits<std::int16_t>::max()),
            std::make_pair("uint16", std::numeric_limits<std::uint16_t>::max() - 1),
            std::make_pair("int32", std::numeric_limits<std::int32_t>::max()),
            std::make_pair("uint32", std::numeric_limits<std::uint32_t>::max() - 1),
            std::make_pair("int64", std::numeric_limits<std::int64_t>::max()),
            std::make_pair("uint64", std::numeric_limits<std::uint64_t>::max() - 1)
        };

        auto iter = Map.find(type);
        assert(iter != Map.end());
        return std::make_pair(iter->second, true);
    }

    try {
        return std::make_pair(std::stoll(value), true);
    } catch(...) {
        return std::make_pair(std::intmax_t(0), false);
    }
}

std::intmax_t Type::builtInIntNullValue(const std::string& type)
{
    static const std::map<std::string, std::intmax_t> Map = {
        std::make_pair("char", 0),
        std::make_pair("std::int8_t", intMinValue("int8", std::string()).first - 1),
        std::make_pair("std::uint8_t", intMaxValue("uint8", std::string()).first + 1),
        std::make_pair("std::int16_t", intMinValue("int16", std::string()).first - 1),
        std::make_pair("std::uint16_t", intMaxValue("uint16", std::string()).first + 1),
        std::make_pair("std::int32_t", intMinValue("int32", std::string()).first - 1),
        std::make_pair("std::uint32_t", intMaxValue("uint32", std::string()).first + 1),
        std::make_pair("std::int64_t", intMinValue("int64", std::string()).first - 1),
        std::make_pair("std::uint64_t", intMaxValue("uint64", std::string()).first + 1),
    };

    auto iter = Map.find(type);
    assert(iter != Map.end());
    return iter->second;
}

} // namespace sbe2comms
