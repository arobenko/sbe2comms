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

#include "Field.h"

#include <iostream>

#include "DB.h"
#include "output.h"
#include "prop.h"

namespace sbe2comms
{

const XmlPropsMap& Field::props(DB& db)
{
    if (!m_props.empty()) {
        return m_props;
    }

    m_props = xmlParseNodeProps(m_node, db.m_doc.get());
    return m_props;
}

bool Field::write(std::ostream& out, DB& db, unsigned indent)
{
    auto& p = props(db);
    auto& name = prop::name(p);
    if (name.empty()) {
        std::cerr << output::indent(1) <<
            "ERROR: Field doesn't provide name." << std::endl;
        return false;
    }

    out << output::indent(indent) << "/// \\brief Definition of \"" << name << "\" field.\n";
    auto& desc = prop::description(p);
    if (!desc.empty()) {
        out << output::indent(indent) << "/// \\details " << desc << '\n';
    }

    return writeImpl(out, db, indent);

}

} // namespace sbe2comms
