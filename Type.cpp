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
        out << output::indent(indent) << "/// @details " << desc << "\n";
    }
}



} // namespace sbe2comms
