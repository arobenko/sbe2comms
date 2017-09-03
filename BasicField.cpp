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

#include "DB.h"
#include "prop.h"
#include "output.h"

namespace sbe2comms
{

namespace
{

bool writeBuiltInType(std::ostream& out, const std::string& type)
{
    static const std::map<std::string, std::string> Map = {
        std::make_pair("int8", "std::int8_t"),
        std::make_pair("uint8", "std::uint8_t"),
        std::make_pair("int16", "std::int16_t"),
        std::make_pair("uint16", "std::uint16_t"),
        std::make_pair("int32", "std::int32_t"),
        std::make_pair("uint32", "std::uint32_t"),
        std::make_pair("int64", "std::uint16_t"),
        std::make_pair("uint64", "std::uint64_t"),
    };

    auto iter = Map.find(type);
    if (iter == Map.end()) {
        return false;
    }

    out << "comms::field::IntValue<field::FieldBase, " << iter->second << ">;\n\n";
    return true;
}

} // namespace

bool BasicField::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
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

    auto typeIter = db.m_types.find(type);
    if (typeIter == db.m_types.end()) {
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

    out << "field::" << type << "<TOpt>;\n\n";
    return true;
}

} // namespace sbe2comms
