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
#include <numeric>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"
#include "common.h"

namespace sbe2comms
{

namespace
{

const std::string MembersSuffix("Members");

enum DataEncIdx
{
    DataEncIdx_length,
    DataEncIdx_data,
    DataEncIdx_numOfValues
};

} // namespace

bool CompositeType::isBundleOptional() const
{
    if (m_members.empty()) {
        return false;
    }

    auto& mem = m_members[0];
    if (mem->getKind() != Kind::Composite) {
        return mem->isOptional();
    }

    return static_cast<const CompositeType&>(*mem).isBundleOptional();
}

bool CompositeType::isValidDimensionType() const
{
    auto verifyMemberFunc =
        [](const Type& t) -> bool
        {
            return t.getKind() == Kind::Basic &&
                   t.getLengthProp() == 1U &&
                   t.isRequired();
        };

    return
        ((m_members.size() == 2) &&
         verifyMemberFunc(*m_members[0]) &&
         verifyMemberFunc(*m_members[1]));
}

CompositeType::Kind CompositeType::getKindImpl() const
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

bool CompositeType::writeImpl(std::ostream& out, unsigned indent)
{
    assert(!m_members.empty());
    for (auto& m : m_members) {
        if (!m->writeDependencies(out, indent)) {
            log::error() << "Failed to write dependencies for composite \"" << getName() << "\"." << std::endl;
            return false;
        }
    }

    if (!checkDataValid()) {
        return false;
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

    if (dataUseRecorded()) {
        return writeData(out, indent);
    }

    return writeBundle(out, indent, hasExtraOpts);
}

std::size_t CompositeType::getSerializationLengthImpl() const
{
    return std::accumulate(
            m_members.begin(), m_members.end(), 0U,
            [](std::size_t soFar, const TypePtr& t)
            {
                return soFar + t->getSerializationLength();
            });
}

bool CompositeType::writeDependenciesImpl(std::ostream& out, unsigned indent)
{
    bool result = true;
    for (auto& m : m_members) {
        result = m->writeDependencies(out, indent) && result;
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
    assert(m_members.empty());
    auto children = xmlChildren(getNode());
    m_members.reserve(children.size());
    unsigned expOffset = 0U;
    unsigned padCount = 0;
    for (auto* c : children) {
        auto mem = Type::create(getDb(), c);
        if (!mem) {
            log::error() << "Failed to create members of \"" << getName() << "\" composite." << std::endl;
            return false;
        }

        std::string cName(reinterpret_cast<const char*>(c->name));
        if (!mem->parse()) {
            log::error() << "Failed to parse \"" << cName  << "\" member of \"" << getName() << "\" composite." << std::endl;
            return false;
        }

        if (!mem->doesExist()) {
            continue;
        }

        do {
            auto offset = mem->getOffset();
            if ((offset == 0U) || (offset == expOffset)) {
                break;
            }

            if (offset < expOffset) {
                log::error() << "Invalid offset of \"" << cName <<
                                "\" member of \"" << getName() << "\" composite, causing overlap.\n" << std::endl;
                return false;
            }

            auto padLen = offset - expOffset;
            ++padCount;
            auto padNode = xmlCreatePadding(padCount, padLen);
            assert(padNode);
            auto padMem = Type::create(getDb(), padNode.get());
            assert(padMem);
            auto* padNodeName = reinterpret_cast<const char*>(padNode->name);
            if (!padMem->parse()) {
                log::error() << "Failed to parse \"" << padNodeName  << "\" member of \"" << getName() << "\" composite." << std::endl;
                return false;
            }

            assert(padMem->getSerializationLength() == padLen);
            expOffset += padLen;
            m_members.push_back(std::move(padMem));
            xmlAddPrevSibling(c, padNode.release());
        } while (false);

        expOffset += mem->getSerializationLength();
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
    auto& n = getReferenceName();
    std::string membersStruct = getName() + MembersSuffix;

    out << output::indent(indent) << "/// \\brief Scope for all the members of the \\ref " << n << " field.\n" <<
           output::indent(indent) << "struct " << membersStruct << '\n' <<
           output::indent(indent) << "{\n";
    bool result = true;
    for (auto& m : m_members) {
        result = m->write(out, indent + 1) && result;
    }

    out << output::indent(indent + 1) << "/// \\ brief Bundling all the defined member types into a single std::tuple.\n";
    if (hasExtraOpts) {
        out << output::indent(indent + 1) << "/// \\tparam TOpt Extra options for list/string fields.\n";
        common::writeExtraOptionsTemplParam(out, indent + 1);
    }
    out << output::indent(indent + 1) << "using All = std::tuple<\n";
    bool first = true;
    for (auto& m : m_members) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        auto& mName = m->getReferenceName();
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

bool CompositeType::writeBundle(std::ostream& out, unsigned indent, bool hasExtraOpts)
{
    writeHeader(out, indent, true);
    common::writeExtraOptionsTemplParam(out, indent);
    out << output::indent(indent) << "struct " << getReferenceName() << " : public\n" <<
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
        auto& mProps = m->getProps();
        out << output::indent(indent + 2) << prop::name(mProps) << std::endl;
    }
    out << output::indent(indent + 1) << ");\n";
    if (isBundleOptional()) {
        out << "\n" <<
               output::indent(indent + 1) << "/// \\brief Check the value of the first member is equivalent to \\b nullValue.\n" <<
               output::indent(indent + 1) << "bool isNull() const\n" <<
               output::indent(indent + 1) << "{\n" <<
               output::indent(indent + 2) << "return field_" << m_members[0]->getName() << "().isNull();\n" <<
               output::indent(indent + 1) << "}\n";
    }
    out << output::indent(indent) << "};\n\n";
    return true;
}

bool CompositeType::writeData(std::ostream& out, unsigned indent)
{
    if (m_members.size() != DataEncIdx_numOfValues) {
        log::error() << "The number of members in \"" << getName() << "\" composite is expected to be " << DataEncIdx_numOfValues << std::endl;
        return false;
    }

    writeHeader(out, indent, true);
    common::writeExtraOptionsTemplParam(out, indent);
    auto& lenMem = *m_members[DataEncIdx_length];
    auto& dataMem = *m_members[DataEncIdx_data];
    out << output::indent(indent) << "using " << getReferenceName() << " = \n" <<
           output::indent(indent + 1) << getName() << MembersSuffix << "::" << dataMem.getReferenceName() << "<\n" <<
           output::indent(indent + 2) << "comms::option::SequenceSizeFieldPrefix<\n" <<
           output::indent(indent + 3) << getName() << MembersSuffix << "::" << lenMem.getReferenceName() << '\n' <<
           output::indent(indent + 2) << ">,\n" <<
           output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">;\n\n";

    return true;
}

bool CompositeType::checkDataValid()
{
    if (!dataUseRecorded()) {
        return true;
    }

    if (m_members.size() != DataEncIdx_numOfValues) {
        log::error() << "The composite \"" << getName() << "\" type has "
                        "been used to encode data field, must have " << DataEncIdx_numOfValues <<
                        " members fields describing length and data. Has " << m_members.size() << std::endl;
        return false;
    }

    if (m_members[DataEncIdx_length]->getKind() != Kind::Basic) {
        log::error() << "The composite \"" << getName() << "\" type has "
                       "been used to encode data field, must have length field of basic type." << std::endl;
        return false;
    }

    if (m_members[DataEncIdx_data]->getKind() != Kind::Basic) {
        log::error() << "The composite \"" << getName() << "\" type has "
                       "been used to encode data field, must have data field of basic type." << std::endl;
        return false;
    }

    if (!m_members[DataEncIdx_data]->hasListOrString()) {
        log::error() << "The composite \"" << getName() << "\" type has "
                       "been used to encode data field, must have data field describing list or string." << std::endl;
        return false;
    }

    if (m_members[DataEncIdx_length]->isOptional()) {
        log::error() << "The composite \"" << getName() << "\" type has "
                       "been used to encode data field, mustn't have optional length field." << std::endl;
        return false;
    }

    return true;

}

} // namespace sbe2comms
