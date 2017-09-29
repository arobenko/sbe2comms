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
#include "log.h"

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

bool CompositeType::parseImpl()
{
    if (!prepareMembers()) {
        return false;
    }

    return true;
}

bool CompositeType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    do {
        if (!dataUseRecorded()) {
            break;
        }

        if (m_members.size() != StringEncIdx_numOfValues) {
            log::error() << "The composite \"" << getName() << "\" type has "
                            "been used to encode data field, must have " << StringEncIdx_numOfValues <<
                            " members fields describing length and data." << std::endl;
            return false;
        }

        if (m_members[StringEncIdx_data]->kind() != Kind::Basic) {
            log::error() << "The composite \"" << getName() << "\" type has "
                           "been used to encode data field, must have data field of basic type." << std::endl;
            return false;
        }

        if (!m_members[StringEncIdx_data]->hasListOrString()) {
            log::error() << "The composite \"" << getName() << "\" type has "
                           "been used to encode data field, must have data field describing list or string-." << std::endl;
            return false;
        }

    } while (false);

    assert(!m_members.empty());
    for (auto& m : m_members) {
        if (!m->writeDependencies(out, db, indent)) {
            log::error() << "Failed to write dependencies for composite \"" << getName() << "\"." << std::endl;
            return false;
        }
    }

    bool hasExtraOpts =
            std::any_of(
                m_members.begin(), m_members.end(),
                [](const TypePtr& m)
                {
                   return m->hasListOrString();
                });

    if (!writeMembers(out, indent, hasExtraOpts)) {
        return false;
    }

    if (mustBeString()) {
        return writeString(out, db, indent);
    }

    return writeBundle(out, db, indent, hasExtraOpts);
}

std::size_t CompositeType::lengthImpl(DB& db)
{
    static_cast<void>(db);
    // TODO
    return 0U;
}

bool CompositeType::writeDependenciesImpl(std::ostream& out, DB& db, unsigned indent)
{
    bool result = true;
    for (auto& m : m_members) {
        result = m->writeDependencies(out, db, indent) && result;
    }
    return result;
}

bool CompositeType::hasListOrStringImpl() const
{
    return std::any_of(
                m_members.begin(), m_members.end(),
                [](const TypePtr& m)
                {
                    return m->hasListOrString();
                });
}

bool CompositeType::prepareMembers()
{
    auto children = xmlChildren(getNode());
    m_members.reserve(children.size());
    for (auto* c : children) {
        std::string cName(reinterpret_cast<const char*>(c->name));
        auto mem = Type::create(cName, getDb(), c);
        if (!mem) {
            log::error() << "Failed to create members of \"" << getName() << "\" composite." << std::endl;
            return false;
        }

        if (!mem->parse()) {
            log::error() << "Failed to parse \"" << cName  << "\" member of \"" << getName() << "\" composite." << std::endl;
            return false;
        }

        if (!mem->doesExist()) {
            continue;
        }

        // TODO: add padding

        m_members.push_back(std::move(mem));
    }

    if (m_members.empty()) {
        log::error() << "The composite \"" << getName() << "\" doesn't define any member types." << std::endl;
        return false;
    }

    return true;
}

bool CompositeType::writeMembers(std::ostream& out, unsigned indent, bool hasExtraOpts)
{
    auto& n = getName();
    std::string membersStruct = n + MembersSuffix;

    out << output::indent(indent) << "/// \\brief Scope for all the members of the \\ref " << n << " field.\n" <<
           output::indent(indent) << "struct " << membersStruct << '\n' <<
           output::indent(indent) << "{\n";
    bool result = true;
    for (auto& m : m_members) {
        result = m->write(out, getDb(), indent + 1) && result;
    }

    out << output::indent(indent + 1) << "/// \\ brief Bundling all the defined member types into a single std::tuple.\n";
    if (hasExtraOpts) {
        out << output::indent(indent + 1) << "/// \\tparam TOpt Extra options for list/string fields.\n";
        writeOptions(out, indent + 1);
    }
    out << output::indent(indent + 1) << "using All = std::tuple<\n";
    bool first = true;
    for (auto& m : m_members) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        auto& mName = m->getName();
        assert(!mName.empty());
        out << output::indent(indent + 2) << mName;
        if (m->hasListOrString()) {
            out << "<TOpt...>";
        }
        else {
            out << "<>";
        }
    }
    out << '\n' <<
           output::indent(indent + 1) << ">;\n" <<
           output::indent(indent) << "};\n\n";
    return result;
}

bool CompositeType::writeBundle(std::ostream& out, DB& db, unsigned indent, bool hasExtraOpts)
{
    writeBrief(out, db, indent, true);
    writeOptions(out, indent);
    out << output::indent(indent) << "struct " << getName() << " : public\n" <<
           output::indent(indent + 1) << "comms::field::Bundle<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << getName() << MembersSuffix << "::All";
    if (hasExtraOpts) {
        out << "<TOpt...>\n";
    }
    else {
        out << ",\n" <<
               output::indent(indent + 2) << "TOpt...\n";
    }
    out << output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Allow access to internal fields.\n" <<
           output::indent(indent + 1) << "/// \\details See definition of \\b COMMS_FIELD_MEMBERS_ACCESS macro\n" <<
           output::indent(indent + 1) << "///     related to \\b comms::field::Bundle class from COMMS library\n" <<
           output::indent(indent + 1) << "///     for details.\\n\n" <<
           output::indent(indent + 1) << "///     The names are:\n";
    auto memsScope = getName() + MembersSuffix + "::";
    for (auto& m : m_members) {
        auto& mProps = m->getProps();
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
    if (m_members.size() != StringEncIdx_numOfValues) {
        log::error() << "The number of members in \"" << getName() << "\" composite is expected to be " << StringEncIdx_numOfValues << std::endl;
        return false;
    }

    writeBrief(out, db, indent, true);
    writeOptions(out, indent);
    auto& lenMem = *m_members[StringEncIdx_length];
    auto& dataMem = *m_members[StringEncIdx_data];
    out << output::indent(indent) << "using " << getName() << " = \n" <<
           output::indent(indent + 1) << getName() << MembersSuffix << "::" << dataMem.getName() << "<\n" <<
           output::indent(indent + 2) << "comms::option::SequenceSizeFieldPrefix<\n" <<
           output::indent(indent + 3) << getName() << MembersSuffix << "::" << lenMem.getName() << '\n' <<
           output::indent(indent + 2) << ">,\n" <<
           output::indent(indent + 2) << "TOpt...\n" <<
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
