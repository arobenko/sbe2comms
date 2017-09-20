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
#include "get.h"
#include "output.h"
#include "BasicType.h"
#include "CompositeType.h"
#include "EnumType.h"
#include "SetType.h"
#include "RefType.h"

namespace sbe2comms
{

Type::Type(xmlNodePtr node)
  : m_node(node)
{
    std::fill(m_uses.begin(), m_uses.end(), 0U);
}

Type::~Type() noexcept = default;

Type::Ptr Type::create(const std::string& name, xmlNodePtr node)
{
    using TypeCreateFunc = std::function<Ptr (xmlNodePtr)>;
    static const std::map<std::string, TypeCreateFunc> Map = {
        std::make_pair(
            "type",
            [](xmlNodePtr n)
            {
                return Ptr(new BasicType(n));
            }),
        std::make_pair(
            "composite",
            [](xmlNodePtr n)
            {
                return Ptr(new CompositeType(n));
            }),
        std::make_pair(
            "enum",
            [](xmlNodePtr n)
            {
                return Ptr(new EnumType(n));
            }),
        std::make_pair(
            "set",
            [](xmlNodePtr n)
            {
                return Ptr(new SetType(n));
            }),
        std::make_pair(
            "ref",
            [](xmlNodePtr n)
            {
                return Ptr(new RefType(n));
            })
    };

    auto createIter = Map.find(name);
    if (createIter == Map.end()) {
        std::cerr << "ERROR: Unknown type kind \"" << name << "\"." << std::endl;
        return Ptr();
    }

    return createIter->second(node);
}

bool Type::write(std::ostream& out, DB& db, unsigned indent)
{
    if (isDeperated(db)) {
        // Don't write anything if type is deprecated
        std::cout << output::indent(indent + 1) <<
                     "INFO: Omitting definition of deprecated \"" << prop::name(props(db)) << "\" type." << std::endl;
        return true;
    }

    if (!isIntroduced(db)) {
        // Don't write anything if type was introduced in later version
        std::cout << output::indent(indent + 1) <<
                     "INFO: Omitting definition of not yet introduced \"" << prop::name(props(db)) << "\" type." << std::endl;
        return true;
    }

    if (m_written) {
        return true;
    }

    if (m_writingInProgress) {
        std::cerr << "ERROR: Recursive types dependencies discovered for \"" << prop::name(props(db)) << "\" type." << std::endl;
        return false;
    }

    m_writingInProgress = true;
    m_written = writeImpl(out, db, indent);
    m_writingInProgress = false;
    return m_written;
}

const XmlPropsMap& Type::props(DB& db)
{
    if (m_props.empty()) {
        m_props = xmlParseNodeProps(m_node, db.m_doc.get());
    }
    return m_props;
}

bool Type::writeDependenciesImpl(std::ostream& out, DB& db, unsigned indent)
{
    static_cast<void>(out);
    static_cast<void>(db);
    static_cast<void>(indent);
    return true;
}

bool Type::isDeperated(DB& db)
{
    auto& p = props(db);
    if (!prop::hasDeprecated(p)) {
        return false;
    }

    auto depVersion = prop::deprecated(p);
    auto currVersion = get::schemaVersion(db);
    return currVersion < depVersion;
}

bool Type::isIntroduced(DB& db)
{
    auto& p = props(db);
    if (!prop::hasSinceVersion(p)) {
        return true;
    }

    auto sinceVersion = prop::sinceVersion(p);
    auto currVersion = get::schemaVersion(db);
    return sinceVersion <= currVersion;
}

void Type::writeBrief(std::ostream& out, DB& db, unsigned indent, bool extraOpts)
{
    auto& p = props(db);
    out << output::indent(indent) << "/// \\brief Definition of \"" << prop::name(p) <<
           "\" field\n";
    auto& desc = prop::description(p);
    if (!desc.empty()) {
        out << output::indent(indent) << "/// \\details " << desc << "\n";
    }

    if (extraOpts) {
        out << output::indent(indent) << "/// \\tparam TOpt Extra options from \\b comms::option namespace.\n";
    }
}

std::string Type::nodeText()
{
    return xmlText(m_node);
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
        return get::emptyString();
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

std::string Type::toString(std::intmax_t val) {
    auto str = std::to_string(val);
    if ((std::numeric_limits<std::int32_t>::max() < val) || (val < std::numeric_limits<std::int32_t>::min())) {
        return str + "LL";
    }

    if ((std::numeric_limits<std::int16_t>::max() < val) || (val < std::numeric_limits<std::int16_t>::min())) {
        return str + "L";
    }

    return str;
}


} // namespace sbe2comms
