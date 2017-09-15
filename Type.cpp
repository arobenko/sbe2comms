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

namespace sbe2comms
{

Type::Type(xmlNodePtr node)
  : m_node(node)
{
    std::fill(m_uses.begin(), m_uses.end(), 0U);
}

Type::~Type() noexcept = default;

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

    return writeImpl(out, db, indent);
}

const XmlPropsMap& Type::props(DB& db)
{
    if (m_props.empty()) {
        m_props = xmlParseNodeProps(m_node, db.m_doc.get());
    }
    return m_props;
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

void Type::writeBrief(std::ostream& out, DB& db, unsigned indent)
{
    auto& p = props(db);
    out << output::indent(indent) << "/// \\brief Definition of \"" << prop::name(p) <<
           "\" field\n";
    auto& desc = prop::description(p);
    if (!desc.empty()) {
        out << output::indent(indent) << "/// \\details " << desc << "\n";
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

} // namespace sbe2comms
