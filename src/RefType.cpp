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

#include "RefType.h"

#include <iostream>

#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"
#include "common.h"
#include "CompositeType.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{
const std::string OptPrefix("TOpt_");
} // namespace

bool RefType::isReferredOptional() const
{
    assert(m_type != nullptr);
    if (m_type->getKind() == Kind::Composite) {
        return asCompositeType(m_type)->isBundleOptional();
    }

    return m_type->isOptional();
}

RefType::Kind RefType::getKindImpl() const
{
    return Kind::Ref;
}

bool RefType::parseImpl()
{
    m_type = getReferenceType();
    if (m_type == nullptr) {
        return false;
    }

    addExtraInclude('\"' + m_type->getName() + ".h\"");
    return true;
}

bool RefType::writeImpl(std::ostream& out, unsigned indent, bool commsOptionalWrapped)
{
    assert(m_type != nullptr);

    if (isBundle()) {
        writeBundle(out, indent, commsOptionalWrapped);
        return true;
    }

    writeHeader(out, indent, commsOptionalWrapped, true);
    common::writeExtraOptionsTemplParam(out, indent);

    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);
    auto typeSuffix = getTypeRefSuffix(commsOptionalWrapped);
    auto typeName = common::refName(m_type->getName(), typeSuffix);

    out << output::indent(indent) << "using " << name << " = " <<
           common::fieldNamespaceStr() << typeName << "<TOpt...>;\n\n";
    return true;
}

bool RefType::writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope)
{
    static_cast<void>(out);
    static_cast<void>(indent);
    static_cast<void>(scope);
    return true;
}

std::size_t RefType::getSerializationLengthImpl() const
{
    assert(m_type != nullptr);
    return m_type->getSerializationLength();
}

bool RefType::hasFixedLengthImpl() const
{
    assert(m_type != nullptr);
    return m_type->hasFixedLength();
}

Type::ExtraOptInfosList RefType::getExtraOptInfosImpl() const
{
    assert(m_type != nullptr);
    auto opts = m_type->getExtraOptInfos();
    for (auto& o : opts) {
        auto newName = getName() + '_' + o.first;
        o.first = newName;
        if (!ba::starts_with(o.second, common::fieldNamespaceStr())) {
            auto newRef = common::fieldNamespaceStr() + o.second;
            o.second = std::move(newRef);
        }
    }
    return opts;
}

bool RefType::writePluginPropertiesImpl(
    std::ostream& out,
    unsigned indent,
    const std::string& scope)
{
    std::string fieldType;
    std::string props;
    scopeToPropertyDefNames(scope, &fieldType, &props);
    auto nameStr = '\"' + getName() + '\"';
    if (scope.empty()) {
        nameStr = common::fieldNameParamNameStr();
    }

    assert(m_type != nullptr);
    auto refPropsStr = "createProps_" + m_type->getName() + "(\"" + getName() + "\")";
    out << output::indent(indent) << "using " << fieldType << " = " <<
           common::scopeFor(getDb().getProtocolNamespace(), common::fieldNamespaceStr() + scope + getName()) <<
           "<>;\n" <<
           output::indent(indent) << fieldType << ' ' << props << "(" << refPropsStr << ");\n\n";

    if (scope.empty()) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }

    return true;
}


Type* RefType::getReferenceType()
{
    auto& p = getProps();
    auto& t = prop::type(p);

    if (t.empty()) {
        log::error() << "Unknown reference type for ref \"" << getName() << "\"." << std::endl;
        return nullptr;
    }

    auto ptr = getDb().findType(t);
    if (ptr == nullptr) {
        log::error() << "Unknown type \"" << t << "\" in ref \"" << getName() << "\"." << std::endl;
    }

    return ptr;
}

bool RefType::isBundle() const
{
    if (m_type->getKind() != Kind::Composite) {
        return false;
    }

    auto* compType = static_cast<const CompositeType*>(m_type);
    return compType->isBundle();
}

void RefType::writeBundle(std::ostream& out, unsigned indent, bool commsOptionalWrapped)
{
    writeHeader(out, indent, commsOptionalWrapped, false);
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);
    auto typeSuffix = getTypeRefSuffix(commsOptionalWrapped);
    auto typeName = common::refName(m_type->getName(), typeSuffix);

    auto allOpts = getExtraOptInfos();
    for (auto& o : allOpts) {
        out << output::indent(indent) << "/// \\tparam " << OptPrefix <<
               o.first << " Extra options for \\ref " << o.second << '\n';
    }
    out << output::indent(indent) << "template<\n";
    for (auto& o : allOpts) {
        out << output::indent(indent + 1) << "typename " << OptPrefix << o.first << common::eqEmptyOptionStr();
        bool comma = (&o != &allOpts.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }
    out << output::indent(indent) << ">\n" <<
           output::indent(indent) << "using " << name << " = " <<
                      common::fieldNamespaceStr() << typeName << "<\n";
    for (auto& o : allOpts) {
        out << output::indent(indent + 1) << OptPrefix << o.first;
        bool comma = (&o != &allOpts.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }
    out << output::indent(indent) << ">;\n\n";
}

const std::string& RefType::getTypeRefSuffix(bool commsOptionalWrapped)
{
    assert(m_type != nullptr);
    if (!m_type->isCommsOptionalWrapped()) {
        return common::emptyString();
    }

    if (commsOptionalWrapped) {
        return common::optFieldSuffixStr();
    }

    auto sinceVersion = getSinceVersion();
    if ((getDb().getMinRemoteVersion() < sinceVersion) &&
        (sinceVersion == m_type->getSinceVersion())) {
        return common::optFieldSuffixStr();
    }

    return common::emptyString();
}

} // namespace sbe2comms
