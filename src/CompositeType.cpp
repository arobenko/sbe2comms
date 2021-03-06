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

#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"
#include "common.h"
#include "BasicType.h"
#include "EnumType.h"
#include "RefType.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{

enum DataEncIdx
{
    DataEncIdx_length,
    DataEncIdx_data,
    DataEncIdx_numOfValues
};

const std::string OptPrefix("TOpt_");

} // namespace

bool CompositeType::isBundleOptional() const
{
    if (m_members.empty()) {
        return false;
    }

    auto& mem = m_members[0];
    if (mem->getKind() == Kind::Composite) {
        return asCompositeType(*mem).isBundleOptional();
    }

    if (mem->getKind() == Kind::Ref) {
        return asRefType(*mem).isReferredOptional();
    }

    return mem->isOptional();
}

bool CompositeType::verifyValidDimensionType() const
{
    auto verifyMemberFunc =
        [this](const Type& t) -> bool
        {
            bool result =
                t.getKind() == Kind::Basic &&
                t.getLengthProp() == 1U &&
                t.isRequired();

            if (!result) {
                log::error() << "The member \"" << t.getName() << "\" of \"" << getName() <<
                                "\" is of invalid format.";
            }
            return result;
        };

    if ((m_members.size() != 2) ||
        (!verifyMemberFunc(*m_members[0])) ||
        (!verifyMemberFunc(*m_members[1]))) {
        return false;
    }

    auto findNameFunc =
        [this](const std::string& tName)
        {
            bool result = std::any_of(
                m_members.begin(), m_members.end(),
                [&tName](const TypePtr& t)
                {
                    return tName == t->getName();
                });

            if (!result) {
                log::error() << "No member of \"" << getName() << "\" has name \"" << tName << "\"." << std::endl;
            }
            return result;
        };

    return findNameFunc(common::blockLengthStr()) &&
           findNameFunc(common::numInGroupStr());
}

bool CompositeType::isValidData() const
{
    auto verifyLengthFunc =
        [](const Type& t) -> bool
        {
            return t.getKind() == Kind::Basic &&
                   t.getLengthProp() == 1U &&
                   t.isRequired() &&
                   (!t.isCommsOptionalWrapped());
        };

    auto verifyDataFunc =
        [](const Type& t) -> bool
        {
            return t.getKind() == Kind::Basic &&
                   t.getLengthProp() == 0U &&
                   t.isRequired();
        };

    return
        ((m_members.size() == DataEncIdx_numOfValues) &&
         verifyLengthFunc(*m_members[0]) &&
         verifyDataFunc(*m_members[1]));
}

bool CompositeType::isBundle() const
{
    return !dataUseRecorded();
}

bool CompositeType::isOpenFramingHeader() const
{
    return (getName() == getDb().getSimpleOpenFramingHeaderTypeName());
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

    if (isMessageHeader()) {
        if (!checkMessageHeader()) {
            return false;
        }

        addExtraInclude(common::localHeader(getDb().getProtocolNamespace(), common::msgIdFileName()));
    }

    if ((isOpenFramingHeader()) && (!checkOpenFramingHeader())) {
        return false;
    }
    
    ExtraIncludes inc;
    for (auto& m : m_members) {
        m->updateExtraIncludes(inc);
    }

    for (auto& i : inc) {
        addExtraInclude(i);
    }

    if (isBundle()) {
        addExtraInclude("\"comms/util/Tuple.h\"");
        addExtraInclude(common::localHeader(getDb().getProtocolNamespace(), common::builtinNamespaceNameStr(), common::versionSetterFileName()));
        addExtraInclude("\"comms/field/Bundle.h\"");
    }

    return true;
}

bool CompositeType::writeImpl(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    assert(!m_members.empty());

    if (!checkDataValid()) {
        return false;
    }

    if (!writeMembers(out, indent)) {
        return false;
    }

    if (dataUseRecorded()) {
        return writeData(out, indent, commsOptionalWrapped);
    }

    return writeBundle(out, indent, commsOptionalWrapped);
}

bool CompositeType::writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope)
{
    std::string membersStruct = getName() + common::memembersSuffixStr();

    out << output::indent(indent) << "/// \\brief Scope for the options of the fields defined in \\ref " << scope << membersStruct << ".\n" <<
           output::indent(indent) << "struct " << membersStruct << '\n' <<
           output::indent(indent) << "{\n";
    bool result = true;
    for (auto& m : m_members) {
        result = m->writeDefaultOptions(out, indent + 1, scope + membersStruct + "::") && result;
    }
    out << output::indent(indent) << "};\n\n";
    return result;
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

bool CompositeType::hasFixedLengthImpl() const
{
    return std::all_of(
                m_members.begin(), m_members.end(),
                [](const TypePtr& m)
                {
                    return m->hasFixedLength();

                });
}

Type::ExtraOptInfosList CompositeType::getExtraOptInfosImpl() const
{
    ExtraOptInfosList list;
    for (auto& m : m_members) {
        auto infos = m->getExtraOptInfos();
        for (auto& i : infos) {
            std::string newName = getName() + '_' + i.first;
            std::string newRef;
            if (ba::starts_with(i.second, common::fieldNamespaceStr())) {
                newRef = i.second;
            }
            else {
                newRef = getName() + common::memembersSuffixStr() + "::" + i.second;
            }
            i = std::make_pair(std::move(newName), std::move(newRef));
        }
        list.splice(list.end(), infos);
    }

    return list;
}

bool CompositeType::writePluginPropertiesImpl(
    std::ostream& out,
    unsigned indent,
    const std::string& scope)
{
    std::string fieldType;
    std::string props;
    scopeToPropertyDefNames(scope, &fieldType, &props);

    bool commsOptionalWrapped = isCommsOptionalWrapped();
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);

    out << output::indent(indent) << "using " << fieldType << " = " <<
           common::scopeFor(getDb().getProtocolNamespace(), common::fieldNamespaceStr() + scope + name) <<
           "<>;\n";

    auto subScope = scope + getName() + common::memembersSuffixStr() + "::";
    auto nameStr = common::fieldNameParamNameStr();
    if (!scope.empty()) {
        nameStr = '\"' + getName() + '\"';
    }

    if (!isBundle()) {
        assert(dataUseRecorded());
        assert(DataEncIdx_numOfValues <= m_members.size());

        auto& varDataMem = m_members[DataEncIdx_data];
        assert(varDataMem);
        assert(varDataMem->getKind() == Kind::Basic);
        bool rawDataArray = asBasicType(*varDataMem).isRawDataArray();
        do {
            if (rawDataArray) {
                out << output::indent(indent) << "auto " << props << " =\n" <<
                       output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType << ">()\n" <<
                       output::indent(indent + 2) << ".name(" << nameStr << ");\n\n";
                break;
            }

            if (!varDataMem->writePluginProperties(out, indent, subScope)) {
                return false;
            }

            std::string memProps;
            common::scopeToPropertyDefNames(subScope, varDataMem->getName(), varDataMem->isCommsOptionalWrapped(), nullptr, &memProps);

            auto& lengthMem = m_members[DataEncIdx_length];
            assert(lengthMem);

            out << output::indent(indent) << "auto " << props << " =\n" <<
                   output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType << ">(\n" <<
                   output::indent(indent + 3) << memProps << ".asMap())\n" <<
                   output::indent(indent + 2) << ".name(" << nameStr << ")\n" <<
                   output::indent(indent + 2) << ".showPrefix()\n" <<
                   output::indent(indent + 2) << ".prefixName(\"" << lengthMem->getName() << "\");\n\n";

        } while (false);


        writeSerialisedHiddenCheck(out, indent, props);

        if (scope.empty() && (!commsOptionalWrapped)) {
            out << output::indent(indent) << "return " << props << ".asMap();\n";
        }

        return true;
    }


    out << output::indent(indent) << "auto " << props << " = \n" <<
           output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType << ">().name(" << nameStr << ");\n\n";

    for (auto& m : m_members) {
        if (!m->writePluginProperties(out, indent, subScope)) {
            return false;
        }

        std::string memProps;
        common::scopeToPropertyDefNames(subScope, m->getName(), false, nullptr, &memProps);
        out << output::indent(indent) << props << ".add(" << memProps << ".asMap());\n\n";
    }

    if (scope.empty() && (!commsOptionalWrapped)) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }

    return true;
}

Type::AliasTemplateArgsList CompositeType::getAliasTemplateArgumentsImpl() const
{
    AliasTemplateArgsList list;
    auto allExtraOpts = getExtraOptInfos();
    for (auto& o : allExtraOpts) {
       list.push_back("TOpt_" + o.first);
    }

    if (dataUseRecorded() && isValidData()) {
        list.push_back("TOpt");
    }
    return list;
}

bool CompositeType::prepareMembers()
{
    assert(m_members.empty());
    auto children = xmlChildren(getNode());
    m_members.reserve(children.size());
    unsigned expOffset = 0U;
    unsigned padCount = 0;
    auto thisSinceVersion = getSinceVersion();
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

        if (mem->getSinceVersion() < thisSinceVersion) {
            log::error() << "Member \"" << mem->getName() << "\" of composite \"" << getName() << "\" has wrong sinceVersion information." << std::endl;
            return false;
        }

        mem->setContainingCompositeVersion(thisSinceVersion);

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

bool CompositeType::writeMembers(std::ostream& out, unsigned indent)
{
    auto& n = getReferenceName();
    std::string membersStruct = getName() + common::memembersSuffixStr();

    out << output::indent(indent) << "/// \\brief Scope for all the members of the \\ref " << n << " field.\n" <<
           output::indent(indent) << "struct " << membersStruct << '\n' <<
           output::indent(indent) << "{\n";
    bool result = true;
    for (auto& m : m_members) {
        result = m->write(out, indent + 1) && result;
    }

    auto allExtraOpts = getAllExtraOpts();

    out << output::indent(indent + 1) << "/// \\brief Bundling all the defined member types into a single std::tuple.\n";
    writeExtraOptsDoc(out, indent + 1, allExtraOpts);
    writeExtraOptsTemplParams(out, indent + 1, allExtraOpts);

    out << output::indent(indent + 1) << "using All = std::tuple<\n";
    for (auto idx = 0U; idx < m_members.size(); ++idx) {
        auto& mem = m_members[idx];
        out << output::indent(indent + 2) << mem->getReferenceName() << '<';
        auto& opt = allExtraOpts[idx];
        if (opt.size() <= 1U) {
            out << OptPrefix << opt.front().first << ">";
        }
        else {
            out << '\n';
            for (auto& o : opt) {
                out << output::indent(indent + 3) << OptPrefix << o.first;
                bool comma = (&o != &opt.back());
                if (comma) {
                    out << ',';
                }
                out << '\n';
            }
            out << output::indent(indent + 2) << ">";
        }

        bool comma = (&mem != &m_members.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }
    out << output::indent(indent + 1) << ">;\n" <<
           output::indent(indent) << "};\n\n";
    return result;
}

bool CompositeType::writeBundle(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    auto extraOpts = getAllExtraOpts();
    for (auto& o : extraOpts) {
        for (auto& internalO : o) {
            if (!ba::starts_with(internalO.second, common::fieldNamespaceStr())) {
                auto newRef = getName() + common::memembersSuffixStr() + "::" + internalO.second;
                internalO.second = std::move(newRef);
            }
        }
    }

    writeBrief(out, indent, commsOptionalWrapped);
    common::writeDetails(out, indent, getDescription());
    writeExtraOptsDoc(out, indent, extraOpts);
    writeExtraOptsTemplParams(out, indent, extraOpts);
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);

    auto writeClassDefFunc =
        [this, &out, &extraOpts](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::Bundle<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << getName() << common::memembersSuffixStr() << "::All<\n";
            for (auto& o : extraOpts) {
                for (auto& internalO : o) {
                    out << output::indent(ind + 2) << OptPrefix << internalO.first;

                    bool comma = ((&o != &extraOpts.back()) || (&internalO != &o.back()));
                    if (comma) {
                        out << ',';
                    }
                    out << '\n';
                }
            }
            out << output::indent(ind + 1) << ">\n" <<
                   output::indent(ind) << ">";

        };

    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n" <<
           output::indent(indent + 1) << "/// \\brief Allow access to internal fields.\n" <<
           output::indent(indent + 1) << "/// \\details See definition of \\b COMMS_FIELD_MEMBERS_ACCESS macro\n" <<
           output::indent(indent + 1) << "///     related to \\b comms::field::Bundle class from COMMS library\n" <<
           output::indent(indent + 1) << "///     for details.\\n\n" <<
           output::indent(indent + 1) << "///     The names are:\n";
    auto memsScope = getName() + common::memembersSuffixStr() + "::";
    for (auto& m : m_members) {
        auto& mProps = m->getProps();
        out << output::indent(indent + 1) << "///     \\li \\b " << prop::name(mProps) << " for \\ref " << memsScope << common::renameKeyword(prop::name(mProps)) << '.' << std::endl;
    }
    out << output::indent(indent + 1) << "COMMS_FIELD_MEMBERS_ACCESS(\n";
    for (auto& m : m_members) {
        auto& mProps = m->getProps();
        out << output::indent(indent + 2) << prop::name(mProps);
        bool comma = (&m != &m_members.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }
    out << output::indent(indent + 1) << ");\n\n" <<
           output::indent(indent + 1) << "/// \\brief Update current message version.\n" <<
           output::indent(indent + 1) << "/// \\details Calls setVersion() of every member.\n" <<
           output::indent(indent + 1) << "/// \\return \\b true if any of the fields returns \\b true.\n" <<
           output::indent(indent + 1) << "bool setVersion(unsigned val)\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "return comms::util::tupleAccumulate(Base::value(), false, " << common::builtinNamespaceStr() << common::versionSetterStr() << "(val));\n" <<
           output::indent(indent + 1) << "}\n";

    if (isBundleOptional()) {
        out << "\n" <<
               output::indent(indent + 1) << "/// \\brief Check the value of the first member is equivalent to \\b nullValue.\n" <<
               output::indent(indent + 1) << "bool isNull() const\n" <<
               output::indent(indent + 1) << "{\n" <<
               output::indent(indent + 2) << "return field_" << m_members[0]->getName() << "().isNull();\n" <<
               output::indent(indent + 1) << "}\n\n" <<
               output::indent(indent + 1) << "/// \\brief Update the value of the first member to be \\b nullValue.\n" <<
               output::indent(indent + 1) << "void setNull()\n" <<
               output::indent(indent + 1) << "{\n" <<
               output::indent(indent + 2) << "field_" << m_members[0]->getName() << "().setNull();\n" <<
               output::indent(indent + 1) << "}\n";
    }

    out << output::indent(indent) << "};\n\n";

    return true;
}

bool CompositeType::writeData(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    if (!isValidData()) {
        log::error() << "The members in \"" << getName() << "\" composite are not defined as expected to implement data fields." << std::endl;
        return false;
    }

    auto allExtraOpts = getAllExtraOpts();
    for (auto& o : allExtraOpts) {
        for (auto& internalO : o) {
            if (!ba::starts_with(internalO.second, common::fieldNamespaceStr())) {
                auto newRef = getName() + common::memembersSuffixStr() + "::" + internalO.second;
                internalO.second = std::move(newRef);
            }
        }
    }

    assert(allExtraOpts.size() == DataEncIdx_numOfValues);
    auto& lengthExtraOpt = allExtraOpts[DataEncIdx_length].front().first;
    auto& dataExtraOpt = allExtraOpts[DataEncIdx_data].front().first;
    writeHeader(out, indent, commsOptionalWrapped, false);
    writeExtraOptsDoc(out, indent, allExtraOpts);
    common::writeExtraOptionsDoc(out, indent);
    writeExtraOptsTemplParams(out, indent, allExtraOpts, true);
    auto& lenMem = *m_members[DataEncIdx_length];
    auto& dataMem = *m_members[DataEncIdx_data];
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);

    auto writeClassDefFunc =
        [this, &out, &lengthExtraOpt, &dataExtraOpt, &dataMem, &lenMem](unsigned ind)
        {
            out << output::indent(ind) << getName() << common::memembersSuffixStr() << "::" << dataMem.getReferenceName() << "<\n" <<
                   output::indent(ind + 1) << "comms::option::SequenceSerLengthFieldPrefix<\n" <<
                   output::indent(ind + 2) << getName() << common::memembersSuffixStr() << "::" << lenMem.getReferenceName() << '<' << OptPrefix << lengthExtraOpt << ">\n" <<
                   output::indent(ind + 1) << ">,\n" <<
                   output::indent(ind + 1) << OptPrefix << dataExtraOpt << ",\n" <<
                   output::indent(ind + 1) << "TOpt\n" <<
                   output::indent(ind) << ">";
        };

    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) <<  "{\n" <<
           output::indent(indent + 1) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "};\n\n";

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

    if (m_members[DataEncIdx_length]->isOptional()) {
        log::error() << "The composite \"" << getName() << "\" type has "
                       "been used to encode data field, mustn't have optional length field." << std::endl;
        return false;
    }

    return true;

}

CompositeType::AllExtraOptInfos CompositeType::getAllExtraOpts() const
{
    AllExtraOptInfos allExtraOpts;
    for (auto& m : m_members) {
        allExtraOpts.push_back(m->getExtraOptInfos());
    }
    assert(allExtraOpts.size() == m_members.size());
    return allExtraOpts;
}

void CompositeType::writeExtraOptsDoc(std::ostream& out, unsigned indent, const AllExtraOptInfos& infos)
{
    for (auto& l : infos) {
        for (auto& o : l) {
            out << output::indent(indent) << "/// \\tparam " << OptPrefix <<
                   o.first << " Extra options for \\ref " << o.second << " from \\b comms::option namespace.\n";
        }
    }
}

void CompositeType::writeExtraOptsTemplParams(
        std::ostream& out,
        unsigned indent,
        const AllExtraOptInfos& infos,
        bool hasExtraOptions)
{
    out << output::indent(indent) << "template<\n";
    for (auto& l : infos) {
        for (auto& o : l) {
            out << output::indent(indent + 1) << "typename " << OptPrefix << o.first << common::eqEmptyOptionStr();
            bool comma = (hasExtraOptions || (&l != &infos.back()) || (&o != &l.back()));
            if (comma) {
                out << ',';
            }
            out << '\n';
        }
    }
    if (hasExtraOptions) {
        out << output::indent(indent + 1) << "typename TOpt" << common::eqEmptyOptionStr() << '\n';
    }
    out << output::indent(indent) << ">\n";
}

bool CompositeType::isMessageHeader() const
{
    return (getName() == getDb().getMessageHeaderType());
}

bool CompositeType::checkMessageHeader()
{
    if (m_members.size() != 4U) {
        log::error() << "Message header composite \"" << getName() << "\" is expected to have 4 members" << std::endl;
        return false;
    }

    auto checkNameFunc =
        [this](const std::string& name)
        {
            bool result =
                std::any_of(
                    m_members.begin(), m_members.end(),
                    [&name](Members::const_reference m)
                    {
                        return m->getName() == name;
                    });
            if (!result) {
                log::error() << "Message header composite \"" << getName() << "\" doesn't have member called \"" << name << "\"." << std::endl;
            }
            return result;
        };

    if ((!checkNameFunc(common::blockLengthStr())) ||
        (!checkNameFunc(common::templateIdStr())) ||
        (!checkNameFunc(common::schemaIdStr())) ||
        (!checkNameFunc(common::versionStr()))) {
        return false;
    }

    for (auto& m : m_members) {
        auto* realM = m->getRealType();
        if (realM->getKind() != Type::Kind::Basic) {
            log::error() << "The member \"" << m->getName() << "\" of message header composite \"" << getName() << "\" " <<
                            "is expected to be of basic type or ref to it";
            return false;
        }

        if (realM->getLengthProp() != 1) {
            log::error() << "The member \"" << m->getName() << "\" of message header composite \"" << getName() << "\" " <<
                            "must have length property equal to 1.";
            return false;
        }

        if (!asBasicType(realM)->isIntType()) {
            return false;
        }
    }

    auto findMemberFunc =
        [this](const std::string& name)
        {
            return std::find_if(
                m_members.begin(), m_members.end(),
                [&name](Members::const_reference m)
                {
                    return m->getName() == name;
                });

        };

    auto schemaIdIter = findMemberFunc(common::schemaIdStr());
    assert(schemaIdIter != m_members.end());
    auto& schemaIdTypePtr = *schemaIdIter;
    assert(schemaIdTypePtr);
    auto* schemaIdRealType = schemaIdTypePtr->getRealType();
    auto schemaIdValue = static_cast<std::intmax_t>(getDb().getSchemaId());
    schemaIdRealType->addExtraOption("comms::option::DefaultNumValue<" + common::num(schemaIdValue) + '>');
    schemaIdRealType->addExtraOption("comms::option::FailOnInvalid<comms::ErrorStatus::ProtocolError>");
    updateRangeOfSchemaId(*schemaIdRealType);

    auto versionIter = findMemberFunc(common::versionStr());
    assert(versionIter != m_members.end());
    auto& versionTypePtr = *versionIter;
    assert(versionTypePtr);
    auto* versionRealType = versionTypePtr->getRealType();
    auto schemaVersionValue = static_cast<std::intmax_t>(getDb().getSchemaVersion());
    versionRealType->addExtraOption("comms::option::DefaultNumValue<" + common::num(schemaVersionValue) + '>');
    updateRangeOfVersion(*versionRealType);

    auto templateIdIter = findMemberFunc(common::templateIdStr());
    assert(templateIdIter != m_members.end());
    auto& templateIdTypePtr = *templateIdIter;
    auto* templateIdRealType = templateIdTypePtr->getRealType();
    assert(templateIdRealType->getKind() == Type::Kind::Basic);
    auto* newNode = getDb().createMsgIdEnumNode(templateIdRealType->getName(), asBasicType(*templateIdRealType).getPrimitiveType());
    auto newTemplateIdTypePtr = Type::create(getDb(), newNode);
    assert(newTemplateIdTypePtr);
    assert(newTemplateIdTypePtr->getKind() == Type::Kind::Enum);
    asEnumType(*newTemplateIdTypePtr).setMessageId();
    if (!newTemplateIdTypePtr->parse()) {
        log::error() << "Failed to parse modified templateId" << std::endl;
        return false;
    }

    if (templateIdTypePtr.get() == templateIdRealType) {
        // templateId is NOT ref to external type
        templateIdTypePtr = std::move(newTemplateIdTypePtr);
        return true;
    }

    // templateId is reference to external type.
    auto oldType = getDb().updateType(templateIdRealType->getName(), std::move(newTemplateIdTypePtr));
    static_cast<void>(oldType);
    if (!templateIdTypePtr->parse()) {
        log::error() << "Failed to re-parse \"" << templateIdTypePtr->getName() << "\" member of \"" << getName() << "\" composite" << std::endl;
        return false;
    }
    return true;
}

bool CompositeType::checkOpenFramingHeader()
{
    assert(isOpenFramingHeader());
    if (m_members.size() != 2U) {
        log::error() << "Simple Open Framing Header \"" << getName() << "\" is expected to have only 2 member types." << std::endl;
        return false;
    }

    auto checkNameFunc =
        [this](const std::string& name)
        {
            bool result =
                std::any_of(
                    m_members.begin(), m_members.end(),
                    [&name](Members::const_reference m)
                    {
                        return m->getName() == name;
                    });
            if (!result) {
                log::error() << "Simple Open Framing Header composite \"" << getName() << "\" doesn't have member called \"" << name << "\"." << std::endl;
            }
            return result;
        };

    if ((!checkNameFunc(common::messageLengthStr())) ||
        (!checkNameFunc(common::encodingTypeStr()))) {
        return false;
    }

    for (auto& m : m_members) {
        auto* realM = m->getRealType();
        if (realM->getKind() != Type::Kind::Basic) {
            log::warning() << "The member \"" << m->getName() << "\" of Simple Open Framing Header composite \"" << getName() << "\" " <<
                            "is not of basic type or ref to it";
            return false;
        }

        if (realM->getLengthProp() != 1) {
            log::error() << "The member \"" << m->getName() << "\" of Simple Open Framing Header composite \"" << getName() << "\" " <<
                            "must have length property equal to 1.";
            return false;
        }

        if (!asBasicType(realM)->isIntType()) {
            return false;
        }
    }

    auto findMemberFunc =
        [this](const std::string& name)
        {
            return std::find_if(
                m_members.begin(), m_members.end(),
                [&name](Members::const_reference m)
                {
                    return m->getName() == name;
                });

        };

    auto messageLengthIter = findMemberFunc(common::messageLengthStr());
    assert(messageLengthIter != m_members.end());
    auto& messageLengthTypePtr = *messageLengthIter;
    assert(messageLengthTypePtr);
    auto* realMessageLengthTypePtr = messageLengthTypePtr->getRealType();

    auto encTypeIter = findMemberFunc(common::encodingTypeStr());
    assert(encTypeIter != m_members.end());
    auto& encTypeTypePtr = *encTypeIter;
    assert(encTypeTypePtr);
    auto* realEncTypeTypePtr = encTypeTypePtr->getRealType();

    auto encTypeLength = encTypeTypePtr->getSerializationLength();
    if (encTypeLength != 2U) {
        log::error() << "The member \"" << common::encodingTypeStr() << "\" of Simple Open Framing Header composite \"" <<
                        getName() << "\" is expected to have 2 bytes serialization length." << std::endl;
        return false;
    }


    auto extraSerLength =
        messageLengthTypePtr->getSerializationLength() +
        encTypeLength;

    auto extraSerLengthStr = common::num(static_cast<std::intmax_t>(extraSerLength));
    realMessageLengthTypePtr->addExtraOption("comms::option::NumValueSerOffset<" + extraSerLengthStr + '>');

    std::uintmax_t sync = 0x5be0;
    if (ba::ends_with(getDb().getEndian(), "LittleEndian")) {
        sync = 0xeb50;
    }

    auto syncStr = common::num(sync);

    auto& encTypeProps = realEncTypeTypePtr->getProps();
    auto& minValueStr = prop::minValue(encTypeProps);
    auto& maxValueStr = prop::maxValue(encTypeProps);

    do {
        if (minValueStr.empty()) {
            xmlSetMinValueProp(realEncTypeTypePtr->getNode(), syncStr);
            break;
        }

        unsigned minValue = 0U;
        try {
            minValue = static_cast<unsigned>(std::stoul(minValueStr));
        }
        catch (...) {
            // do nothing;
        }

        if (minValue != sync) {
            log::error() << "Invalid minValue attribute of \"" << realEncTypeTypePtr->getName() << "\" member of Simple Open Frame Header." << std::endl;
            return false;
        }

    } while (false);

    do {
        if (maxValueStr.empty()) {
            xmlSetMaxValueProp(realEncTypeTypePtr->getNode(), syncStr);
            break;
        }

        unsigned maxValue = 0U;
        try {
            maxValue = static_cast<unsigned>(std::stoul(maxValueStr));
        }
        catch (...) {
            // do nothing;
        }

        if (maxValue != sync) {
            log::error() << "Invalid maxValue attribute of \"" << realEncTypeTypePtr->getName() << "\" member of Simple Open Frame Header." << std::endl;
            return false;
        }

    } while (false);

    realEncTypeTypePtr->updateNodeProperties();
    realEncTypeTypePtr->addExtraOption("comms::option::FailOnInvalid<comms::ErrorStatus::ProtocolError>");

    return true;
}

bool CompositeType::updateRangeOfSchemaId(Type& schemaId)
{
    auto& schemaIdProps = schemaId.getProps();
    auto& minSchemaIdStr = prop::minValue(schemaIdProps);
    auto& maxSchemaIdStr = prop::maxValue(schemaIdProps);
    auto schemaIdStr = common::num(static_cast<std::intmax_t>(getDb().getSchemaId()));

    do {
        if (minSchemaIdStr.empty()) {
            xmlSetMinValueProp(schemaId.getNode(), schemaIdStr);
            break;
        }

        unsigned minValue = 0U;
        try {
            minValue = static_cast<unsigned>(std::stoul(minSchemaIdStr));
        }
        catch (...) {
            // do nothing;
        }

        if (minValue != getDb().getSchemaId()) {
            log::error() << "Invalid minValue attribute of \"" << schemaId.getName() << "\" member of Simple Open Frame Header." << std::endl;
            return false;
        }

    } while (false);

    do {
        if (maxSchemaIdStr.empty()) {
            xmlSetMaxValueProp(schemaId.getNode(), schemaIdStr);
            break;
        }

        unsigned maxValue = 0U;
        try {
            maxValue = static_cast<unsigned>(std::stoul(maxSchemaIdStr));
        }
        catch (...) {
            // do nothing;
        }

        if (maxValue != getDb().getSchemaId()) {
            log::error() << "Invalid maxValue attribute of \"" << schemaId.getName() << "\" member of Simple Open Frame Header." << std::endl;
            return false;
        }

    } while (false);

    schemaId.updateNodeProperties();
    return true;
}

bool CompositeType::updateRangeOfVersion(Type& version)
{
    unsigned minVersion = getDb().getMinRemoteVersion();
    auto& versionProps = version.getProps();
    auto& minVersionStr = prop::minValue(versionProps);
    auto versionStr = common::num(static_cast<std::intmax_t>(minVersion));

    do {
        if (minVersionStr.empty()) {
            xmlSetMinValueProp(version.getNode(), versionStr);
            break;
        }

        unsigned minValue = 0U;
        try {
            minValue = static_cast<unsigned>(std::stoul(versionStr));
        }
        catch (...) {
            // do nothing;
        }

        if (minValue != minVersion) {
            log::error() << "Invalid minValue attribute of \"" << version.getName() << "\" member of Simple Open Frame Header." << std::endl;
            return false;
        }

    } while (false);

    version.updateNodeProperties();
    return true;
}


} // namespace sbe2comms
