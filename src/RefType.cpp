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

    return true;
}

bool RefType::writeImpl(std::ostream& out, unsigned indent)
{
    assert(m_type != nullptr);
    assert(m_type->isWritten());

    if (isBundle()) {
        writeBundle(out, indent);
        return true;
    }

    writeHeader(out, indent, true);
    common::writeExtraOptionsTemplParam(out, indent);
    out << output::indent(indent) << "using " << getReferenceName() << " = " <<
           common::fieldNamespaceStr() << m_type->getReferenceName() << "<TOpt...>;\n\n";
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

bool RefType::writeDependenciesImpl(std::ostream& out, unsigned indent)
{
    if (m_type == nullptr) {
        return false;
    }

    return m_type->write(out, indent);
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
        if (!ba::starts_with(o.second, common::fieldNamespaceStr())) {
            auto newRef = common::fieldNamespaceStr() + o.second;
            o.second = std::move(newRef);
        }
    }
    return opts;
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

void RefType::writeBundle(std::ostream& out, unsigned indent)
{
    writeHeader(out, indent, false);
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
           output::indent(indent) << "using " << getReferenceName() << " = " <<
                      common::fieldNamespaceStr() << m_type->getReferenceName() << "<\n";
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

} // namespace sbe2comms
