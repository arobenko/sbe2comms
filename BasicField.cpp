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

#include "DB.h"
#include "prop.h"
#include "output.h"

namespace sbe2comms
{

namespace
{

bool writeBuiltInTypeInt(std::ostream& out, const std::string& type)
{
    static const std::set<std::string> Set = {
        "int8", "uint8", "int16", "uint16",
        "int32", "uint32", "int64", "uint64"
    };

    auto iter = Set.find(type);
    if (iter == Set.end()) {
        return false;
    }

    out << "sbe2comms::" << *iter << "<field::FieldBase>;\n\n";
    return true;
}

bool writeBuiltInTypeFloat(std::ostream& out, const std::string& type)
{
    static const std::set<std::string> Set = {
        "float", "double"
    };

    auto iter = Set.find(type);
    if (iter == Set.end()) {
        return false;
    }

    out << "sbe2comms::" << *iter << "Field<field::FieldBase>;\n\n";
    return true;
}

bool writeBuiltInType(std::ostream& out, const std::string& type)
{
    return writeBuiltInTypeInt(out, type) || writeBuiltInTypeFloat(out, type);
}

} // namespace

bool BasicField::writeImpl(std::ostream& out, DB& db, unsigned indent)
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
            "ERROR: Field \"" << name << "\" does NOT specify type." << std::endl;
        return false;
    }

    auto& types = db.getTypes();
    auto typeIter = types.find(type);
    if (typeIter == types.end()) {
        bool builtIn = writeBuiltInType(out, type);
        if (builtIn) {
            return true;
        }

        out << " ???;\n\n";
        std::cerr << output::indent(1) <<
            "ERROR: Unknown type \"" << type << "\" for field \"" << name << "\"" << std::endl;
        return false;
    }

    // TODO: check presence

    assert(typeIter->second);
    typeIter->second->recordNormalUse();

    out << '\n' <<
        output::indent(indent + 1) << "field::" << type << "<\n" <<
        output::indent(indent + 2) << extraOptionsString(db) << '\n' <<
        output::indent(indent + 1) << ">;\n\n";
    return true;
}

} // namespace sbe2comms
