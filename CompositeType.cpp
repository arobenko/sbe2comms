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

#include "CompositeType.h"

#include <iostream>

#include "DB.h"
#include "prop.h"
#include "output.h"

namespace sbe2comms
{

namespace
{

const std::string MembersSuffix("Members");

enum StringEncIdx
{
    StringEncIdx_length,
    StringEncIdx_data,
    StringEncIdx_numOfValues
};

} // namespace

CompositeType::Kind CompositeType::kindImpl() const
{
    return Kind::Composite;
}

bool CompositeType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    return true; // TODO: remove

    if (!prepareMembers(db)) {
        return false;
    }

    auto& p = props(db);
    for (auto& m : m_members) {
        if (!m->writeDependencies(out, db, indent)) {
            std::cerr << "ERROR: Failed to write dependencies for composite \"" << prop::name(p) << "\"." << std::endl;
            return false;
        }
    }

    if (!writeMembers(out, db, indent)) {
        return false;
    }

    if (mustBeString()) {
        return writeString(out, db, indent);
    }

    return writeBundle(out, db, indent);
}

std::size_t CompositeType::lengthImpl(DB& db)
{
    static_cast<void>(db);
    // TODO
    return 0U;
}

bool CompositeType::prepareMembers(DB& db)
{
    auto& p = props(db);
    auto children = xmlChildren(getNode());
    m_members.reserve(children.size());
    for (auto* c : children) {
        std::string cName(reinterpret_cast<const char*>(c->name));
        m_members.push_back(Type::create(cName, getDb(), c));
        if (!m_members.back()) {
            m_members.pop_back();
            std::cerr << "ERROR: Failed to create members of \"" << prop::name(p) << "\" composite." << std::endl;
            return false;
        }
    }
    return !m_members.empty();
}

bool CompositeType::writeMembers(std::ostream& out, DB& db, unsigned indent)
{
    auto& p = props(db);
    auto& n = prop::name(p);
    std::string membersStruct = n + MembersSuffix;

    out << output::indent(indent) << "/// \\brief Scope for all the members of the \\ref " << n << " field.\n" <<
           output::indent(indent) << "struct " << membersStruct << '\n' <<
           output::indent(indent) << "{\n";
    bool result = true;
    for (auto& m : m_members) {
        result = m->write(out, db, indent + 1) && result;
    }

    out << output::indent(indent + 1) << "/// \\ brief Bundling all the defined member types into a single std::tuple.\n" <<
           output::indent(indent + 1) << "using All = std::tuple<\n";
    bool first = true;
    for (auto& m : m_members) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        auto& mProps = m->props(db);
        auto& mName = prop::name(mProps);
        if (!mName.empty()) {
            out << output::indent(indent + 2) << mName;
        }
    }
    out << '\n' <<
           output::indent(indent + 1) << ">;\n" <<
           output::indent(indent) << "};\n\n";
    return result;
}

bool CompositeType::writeBundle(std::ostream& out, DB& db, unsigned indent)
{
    writeBrief(out, db, indent);
    auto& p = props(db);
    auto& n = prop::name(p);
    if (n.empty()) {
        std::cerr << "ERROR: Unknown name of composite type" << std::endl;
        return false;
    }

    out << output::indent(indent) << "struct " << n << " : public\n" <<
           output::indent(indent + 1) << "comms::field::Bundle<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << n << MembersSuffix << "::All\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Allow access to internal fields.\n" <<
           output::indent(indent + 1) << "/// \\details See definition of \\b COMMS_FIELD_MEMBERS_ACCESS macro\n" <<
           output::indent(indent + 1) << "///     related to \\b comms::field::Bundle class from COMMS library\n" <<
           output::indent(indent + 1) << "///     for details.\\n\n" <<
           output::indent(indent + 1) << "///     The names are:\n";
    auto memsScope = n + MembersSuffix + "::";
    for (auto& m : m_members) {
        auto& mProps = m->props(db);
        out << output::indent(indent + 1) << "///     \\li \\b " << prop::name(mProps) << " for \\ref " << memsScope << prop::name(mProps) << '.' << std::endl;
    }
    out << output::indent(indent + 1) << "COMMS_FIELD_MEMBERS_ACCESS(\n";
    bool first = true;
    for (auto& m : m_members) {
        if (!first) {
            out << ",\n";
        }
        auto& mProps = m->props(db);
        out << output::indent(indent + 2) << prop::name(mProps) << std::endl;
    }
    out << output::indent(indent + 1) << ");\n" <<
           output::indent(indent) << "};\n\n";

    return true;
}

bool CompositeType::writeString(std::ostream& out, DB& db, unsigned indent)
{
    auto& p = props(db);
    auto& n = prop::name(p);

    if (m_members.size() != StringEncIdx_numOfValues) {
        std::cerr << "ERROR: The number of members in \"" << n << "\" composite is expected to be " << StringEncIdx_numOfValues << std::endl;
        return false;
    }

    writeBrief(out, db, indent, true);
    auto& lenProps = m_members[StringEncIdx_length]->props(db);
    auto& dataProps = m_members[StringEncIdx_data]->props(db);
    out << output::indent(indent) << "template <typename... TOpt>\n" <<
           output::indent(indent) << "using " << n << " = \n" <<
           output::indent(indent + 1) << n << MembersSuffix << "::" << prop::name(dataProps) << "<\n" <<
           output::indent(indent + 2) << "comms::option::SequenceSizeFieldPrefix<\n" <<
           output::indent(indent + 3) << n << MembersSuffix << "::" << prop::name(lenProps) << '\n' <<
           output::indent(indent + 2) << ">\n" <<
           output::indent(indent + 1) << ">;\n\n";
    return true;
}

bool CompositeType::mustBeString()
{
    if (!dataUseRecorded()) {
        return false;
    }

    // TODO: check semantic type(s)
    return true;
}

} // namespace sbe2comms
