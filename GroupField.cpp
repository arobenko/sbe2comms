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

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "get.h"
#include "log.h"
#include "BasicField.h"

namespace sbe2comms
{

namespace
{
const std::string MembersSuffix("Members");

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

    return true;
}

bool GroupField::writeImpl(std::ostream& out, DB& db, unsigned indent, const std::string& suffix)
{
    static_cast<void>(db);
    static_cast<void>(suffix);

    bool hasExtraOpts =
            std::any_of(
                m_members.begin(), m_members.end(),
                [](const FieldPtr& m)
                {
                   return m->hasListOrString();
                });

    if (!writeMembers(out, indent, hasExtraOpts)) {
        return false;
    }

    // TODO:
    return true;

    bool result = false;
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

bool GroupField::prepareMembers()
{
    assert(m_members.empty());
    auto children = xmlChildren(getNode());
    m_members.reserve(children.size());
    unsigned expOffset = 0U;
    unsigned padCount = 0;
    bool rootBlock = true;
    bool dataMembers = false;
    for (auto* c : children) {
        auto mem = Field::create(getDb(), c, std::string());
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
            log::error() << "Basic member \"" << cName << "\" of \"" << getName() << "\" group cannot follow other group or data" << std::endl;
            return false;
        }

        if ((dataMembers) && (mem->getKind() != Kind::Data)) {
            log::error() << "member \"" << cName << "\" of \"" << getName() << "\" group cannot follow other group or data" << std::endl;
            return false;
        }

        if (mem->getKind() == Kind::Data) {
            rootBlock = false;
            dataMembers = true;
        }

        do {
            if (!rootBlock) {
                break;
            }

            auto offset = mem->getOffset();
            if (mem->getKind() != Kind::Basic) {
                rootBlock = false;
                offset = std::max(offset, getBlockLength());
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
            ++padCount;
            auto* padType = getDb().getPaddingType(padLen);
            if (padType == nullptr) {
                log::error() << "Failed to generate padding type for \"" << getName() << "\" group." << std::endl;
                return false;
            }

            auto padNode = xmlCreatePaddingField(padCount, padType->getName());
            assert(padNode);
            auto padMem = Field::create(getDb(), padNode.get(), std::string());
            assert(padMem);
            assert(padMem->getKind() == Field::Kind::Basic);
            auto* castedPadMem = static_cast<BasicField*>(padMem.get());
            castedPadMem->setGeneratedPadding();

            auto* padNodeName = reinterpret_cast<const char*>(padNode->name);
            if (!padMem->parse()) {
                log::error() << "Failed to parse \"" << padNodeName  << "\" member of \"" << getName() << "\" group." << std::endl;
                return false;
            }

            assert(castedPadMem->getSerializationLength() == padLen);
            expOffset += padLen;
            m_members.push_back(std::move(padMem));
            xmlAddPrevSibling(c, padNode.release());
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

    return true;
}

unsigned GroupField::getBlockLength() const
{
    return prop::blockLength(getProps());
}

bool GroupField::writeMembers(std::ostream& out, unsigned indent, bool hasExtraOpts)
{
    auto& n = getName();
    std::string membersStruct = getName() + MembersSuffix;

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
    }
    out << '\n' <<
           output::indent(indent + 1) << ">;\n" <<
           output::indent(indent) << "};\n\n";
    return result;
}


} // namespace sbe2comms
