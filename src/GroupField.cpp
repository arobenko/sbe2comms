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
#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"
#include "BasicField.h"
#include "CompositeType.h"
#include "common.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{


} // namespace

Field::Kind GroupField::getKindImpl() const
{
    return Kind::Group;
}

bool GroupField::parseImpl()
{
    if (!prepareMembers()) {
        return false;
    }

    auto& dimType = getDimensionType();
    m_type = getDb().findType(dimType);
    if (m_type == nullptr) {
        log::error() << "Failed to find dimentionType \"" << dimType << "\" for group \"" << getName() << "\"." << std::endl;
        return false;
    }

    if (m_type->getKind() != Type::Kind::Composite) {
        log::error() << "The dimentionType \"" << dimType << "\" of group \"" << getName() << "\" must be composite." << std::endl;
        return false;
    }

    auto* compType = asCompositeType(m_type);
    if (!compType->verifyValidDimensionType()) {
        log::error() << "The dimentionType \"" << dimType << "\" of group \"" << getName() << "\" is not of valid format." << std::endl;
        return false;
    }

    getDb().recordGroupListUsage();
    return true;
}

bool GroupField::writeImpl(std::ostream& out, unsigned indent, const std::string& suffix)
{
    static_cast<void>(suffix);

    if (!writeMembers(out, indent)) {
        return false;
    }

    writeBundle(out, indent);
    writeHeader(out, indent, suffix);

    unsigned basicFieldCount =
        std::count_if(
            m_members.begin(), m_members.end(),
            [](const FieldPtr& m) -> bool
            {
                return m->getKind() == Kind::Basic;
            });

    assert(m_type != nullptr);
    auto extraOpts = m_type->getExtraOptInfos();

    out << output::indent(indent) << "using " << getName() << " =\n" <<
           output::indent(indent + 1) << common::builtinNamespaceStr() << common::groupListStr() << "<\n" <<
           output::indent(indent + 2) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << getName() << common::elementSuffixStr() << ",\n" <<
           output::indent(indent + 2) << common::fieldNamespaceStr() << getDimensionType() << "<\n";
    for (auto& o : extraOpts) {
        out << output::indent(indent + 3) << common::optParamPrefixStr();
        if (!ba::starts_with(o.second, common::fieldNamespaceStr())) {
            out << common::fieldNamespaceStr();
        }
        out << o.second;
        bool comma = (&o != &extraOpts.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }
    out << output::indent(indent + 2) << ">,\n" <<
           output::indent(indent + 2) << basicFieldCount << ",\n" <<
           output::indent(indent + 2) << getFieldOptString() << '\n' <<
           output::indent(indent + 1) << ">;\n\n";
    return true;
}

bool GroupField::usesBuiltInTypeImpl() const
{
    return true;
}

bool GroupField::writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope)
{
    return writeMembersDefaultOptions(out, indent, scope) &&
           Base::writeDefaultOptionsImpl(out, indent, scope);
}

bool GroupField::prepareMembers()
{
    assert(m_members.empty());
    auto children = xmlChildren(getNode());
    m_members.reserve(children.size());
    unsigned expOffset = 0U;
    unsigned padCount = 0;
    bool rootBlock = true;
    bool dataMembers = false;
    auto blockLength = getBlockLength();
    auto scope = getScope() + getName() + common::memembersSuffixStr() + "::";
    auto addPaddingFunc =
        [this, &padCount, &expOffset, &scope](xmlNodePtr c, unsigned padLen, bool before = true) -> bool
        {
            ++padCount;
            auto* padType = getDb().getPaddingType(padLen);
            if (padType == nullptr) {
                log::error() << "Failed to generate padding type for \"" << getName() << "\" message." << std::endl;
                return false;
            }

            auto padNode = xmlCreatePaddingField(padCount, padType->getName());
            assert(padNode);
            auto padField = Field::create(getDb(), padNode.get(), scope);
            assert(padField);
            assert(padField->getKind() == Field::Kind::Basic);
            auto* castedPadMem = static_cast<BasicField*>(padField.get());
            castedPadMem->setGeneratedPadding();

            auto* padNodeName = reinterpret_cast<const char*>(padNode->name);
            if (!padField->parse()) {
                log::error() << "Failed to parse \"" << padNodeName  << "\" field of \"" << getName() << "\" message." << std::endl;
                return false;
            }

            assert(castedPadMem->getSerializationLength() == padLen);
            expOffset += padLen;
            m_members.push_back(std::move(padField));
            if (before) {
                assert(c != nullptr);
                xmlAddPrevSibling(c, padNode.release());
            }
            else if (c != nullptr) {
                xmlAddNextSibling(c, padNode.release());
            }
            else {
                xmlAddChild(getNode(), padNode.release());
            }
            return true;
        };

    for (auto* c : children) {
        auto mem = Field::create(getDb(), c, scope);
        if (!mem) {
            log::error() << "Failed to create members of \"" << getName() << "\" group." << std::endl;
            return false;
        }

        std::string cName(reinterpret_cast<const char*>(c->name));
        if (!mem->parse()) {
            log::error() << "Failed to parse \"" << cName  << "\" member of \"" << getName() << "\" group." << std::endl;
            return false;
        }

        if (!mem->doesExist()) {
            continue;
        }

        if ((!rootBlock) && (mem->getKind() == Kind::Basic)) {
            log::error() << "Basic member \"" << mem->getName() << "\" of \"" << getName() << "\" group cannot follow other group or data" << std::endl;
            return false;
        }

        if ((dataMembers) && (mem->getKind() != Kind::Data)) {
            log::error() << "member \"" << mem->getName() << "\" of \"" << getName() << "\" group cannot follow other group or data" << std::endl;
            return false;
        }

        if (mem->getKind() == Kind::Data) {
            dataMembers = true;
        }

        do {
            if (!rootBlock) {
                break;
            }

            auto offset = mem->getOffset();
            if (mem->getKind() != Kind::Basic) {
                rootBlock = false;
                offset = std::max(offset, blockLength);
            }

            if ((blockLength != 0) && (blockLength < offset)) {
                log::error() << "Invalid offset of \"" << mem->getName() << "\" or blockLength is to small." << std::endl;
                return false;
            }

            if ((offset == 0U) || (offset == expOffset)) {
                break;
            }

            if (offset < expOffset) {
                log::error() << "Invalid offset of \"" << cName <<
                                "\" member of \"" << getName() << "\" group, causing overlap.\n" << std::endl;
                return false;
            }


            auto padLen = offset - expOffset;
            if (!addPaddingFunc(c, padLen)) {
                return false;
            }
        } while (false);

        if (rootBlock) {
            assert(mem->getKind() == Kind::Basic);
            expOffset += static_cast<const BasicField*>(mem.get())->getSerializationLength();
        }
        m_members.push_back(std::move(mem));
    }

    if (m_members.empty()) {
        log::error() << "The composite \"" << getName() << "\" doesn't define any member types." << std::endl;
        return false;
    }

    if (rootBlock && (blockLength != 0) && (expOffset < blockLength)) {
        xmlNodePtr c = nullptr;
        if (!m_fields.empty()) {
            c = m_fields.back()->getNode();
        }
        return addPaddingFunc(c, blockLength - expOffset);
    }

    return true;
}

unsigned GroupField::getBlockLength() const
{
    return prop::blockLength(getProps());
}

bool GroupField::writeMembers(std::ostream& out, unsigned indent)
{
    auto& n = getName();
    std::string membersStruct = getName() + common::memembersSuffixStr();

    out << output::indent(indent) << "/// \\brief Scope for all the members of the \\ref " << n << " field.\n" <<
           output::indent(indent) << "struct " << membersStruct << '\n' <<
           output::indent(indent) << "{\n";
    bool result = true;
    for (auto& m : m_members) {
        result = m->write(out, indent + 1) && result;
    }

    out << output::indent(indent + 1) << "/// \\brief Bundling all the defined member types into a single std::tuple.\n";
    out << output::indent(indent + 1) << "using All = std::tuple<\n";
    for (auto& m : m_members) {
        auto& mName = m->getName();
        assert(!mName.empty());
        out << output::indent(indent + 2) << mName;
        bool comma = (&m != &m_members.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }
    out << output::indent(indent + 1) << ">;\n" <<
           output::indent(indent) << "};\n\n";
    return result;
}

void GroupField::writeBundle(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "/// \\brief Element of \\ref " << getName() << " list.\n";

    out << output::indent(indent) << "struct " << getName() << common::elementSuffixStr() << " : public\n" <<
           output::indent(indent + 1) << "comms::field::Bundle<\n" <<
           output::indent(indent + 2) << common::fieldNamespaceStr() << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << "typename " << getName() << common::memembersSuffixStr() << "::All";

    out << '\n' <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n"<<
           output::indent(indent + 1) << "/// \\brief Allow access to internal fields.\n" <<
           output::indent(indent + 1) << "/// \\details See definition of \\b COMMS_FIELD_MEMBERS_ACCESS macro\n" <<
           output::indent(indent + 1) << "///     related to \\b comms::field::Bundle class from COMMS library\n" <<
           output::indent(indent + 1) << "///     for details.\n" <<
           output::indent(indent + 1) << "COMMS_FIELD_MEMBERS_ACCESS(\n";

    for (auto& m : m_members) {
        out << output::indent(indent + 2) << m->getName();
        bool comma = (&m != &m_members.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }

    out << output::indent(indent + 1) << ");\n" <<
           output::indent(indent) << "};\n\n";
}

const std::string& GroupField::getDimensionType() const
{
    return prop::dimensionType(getProps());
}

bool GroupField::writeMembersDefaultOptions(std::ostream& out, unsigned indent, const std::string& scope)
{
    out << output::indent(indent) << "/// \\brief Scope for the options of \\ref " << scope << getName() << " field members.\n" <<
           output::indent(indent) << "struct " << getName() << common::memembersSuffixStr() << '\n' <<
           output::indent(indent) << "{\n";
    auto newScope = scope + getName() + common::memembersSuffixStr() + "::";
    bool result = true;
    for (auto& m : m_members) {
        result = m->writeDefaultOptions(out, indent + 1, newScope) && result;
    }
    out << output::indent(indent) << "};\n\n";
    return result;
}


} // namespace sbe2comms
