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

#include "GroupField.h"

#include <iostream>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "get.h"

namespace sbe2comms
{

bool GroupField::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    if (!createFields(db)) {
        return false;
    }

    bool result = true;
    for (auto& f : m_fields) {
        result = f->write(out, db, indent) && result;
    }

    auto& p = props(db);
    auto& name = prop::name(p);
    if (name.empty()) {
        std::cerr << "ERROR: Unknown name for group field" << std::endl;
        return false;
    }

    std::string elemName(name + "Element");
    out << output::indent(indent) << "/// \\brief Definition of the element type for the \\ref \"" << name << "\" list.\n";
    out << output::indent(indent) << "struct " << elemName << " : public \n" <<
           output::indent(indent + 1) << "comms::field::Bundle<\n" <<
           output::indent(indent + 2) << "field::FieldBase,\n" <<
           output::indent(indent + 2) << "std::tuple<\n";

    auto listFieldsFunc =
        [this, indent, &out, &db](unsigned extraIndent)
        {
            bool firstField = true;
            for (auto& f : m_fields) {
                auto& fp = f->props(db);
                auto& fieldName = prop::name(fp);
                assert(!fieldName.empty());
                if (!firstField) {
                    out << ",\n";
                }
                else {
                    firstField = false;
                }
                out << output::indent(indent + extraIndent) << fieldName;
            }
            out << "\n";
        };

    listFieldsFunc(3);

    out << output::indent(indent + 2) << ">\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Allow access to internal fields.\n" <<
           output::indent(indent + 1) << "/// \\details See definition of \\b COMMS_FIELD_MEMBERS_ACCESS macro\n" <<
           output::indent(indent + 1) << "///     related to \\b comms::field::Bundle class from COMMS library\n" <<
           output::indent(indent + 1) << "///     for details.\n" <<
           output::indent(indent + 1) << "COMMS_FIELD_MEMBERS_ACCESS(\n";
    listFieldsFunc(2);
    out << output::indent(indent + 1) << ");\n" <<
           output::indent(indent) << "};\n\n";

    result = startWrite(out, db, indent) && result;
    out << output::indent(indent) << "using " << name << " = \n" <<
           output::indent(indent + 1) << "sbe2comms::groupList<\n" <<
           output::indent(indent + 2) << "field::FieldBase,\n" <<
           output::indent(indent + 2) << elemName << ",\n" <<
           output::indent(indent + 2) << extraOptionsString(db) << '\n' <<
           output::indent(indent + 1) << ">;\n\n";
    return result;
}

bool GroupField::createFields(DB& db)
{
    if (!m_fields.empty()) {
        return true;
    }

    auto* node = getNode();
    assert(node != nullptr);
    auto* child = node->children;
    while (child != nullptr) {
        if (child->type == XML_ELEMENT_NODE) {
            auto fieldPtr = Field::create(db, child, getMsgName());
            if (!fieldPtr) {
                std::cerr << "ERROR: Unknown field kind \"" << child->name << "\"!" << std::endl;
                return false;
            }

            if (!fieldPtr->parse()) {
                std::cerr << "ERROR: Failed to parse \"" << child->name << "\"!" << std::endl;
            }

            if (!insertField(std::move(fieldPtr), db)) {
                return false;
            }
        }

        child = child->next;
    }

    if (m_fields.empty()) {
        std::cerr << "ERROR: Group " << node->name << " does NOT have any member fields" << std::endl;
        return false;
    }

    return true;
}

bool GroupField::insertField(FieldPtr field, DB& db)
{

    static_cast<void>(db);
    // TODO: do padding

    m_fields.push_back(std::move(field));
    return true;
}

} // namespace sbe2comms
