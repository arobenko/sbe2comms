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

#include "DataField.h"

#include <iostream>

#include "DB.h"
#include "prop.h"
#include "output.h"

namespace sbe2comms
{

bool DataField::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    if (!startWrite(out, db, indent)) {
        return false;
    }

    auto& p = props(db);
    auto& name = prop::name(p);
    assert(!name.empty());

    out << output::indent(indent) << "using " << name << " = ";

    auto& type = prop::type(p);
    if (type.empty()) {
        out << " ???;\n\n";
        std::cerr << output::indent(1) <<
            "ERROR: Data field \"" << name << "\" does NOT specify type." << std::endl;
        return false;
    }

    auto typeIter = db.m_types.find(type);
    if (typeIter == db.m_types.end()) {
        out << " ???;\n\n";
        std::cerr << output::indent(1) <<
            "ERROR: Unknown type \"" << type << "\" for data field \"" << name << "\"" << std::endl;
        return false;
    }

    // TODO: check presence

    assert(typeIter->second);
    typeIter->second->recordDataUse();

    out << "field::" << type << "<TOpt>;\n\n";
    return true;
}

} // namespace sbe2comms
