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

#include "DataField.h"

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

Field::Kind DataField::getKindImpl() const
{
    return Kind::Data;
}

unsigned DataField::getReferencedTypeSinceVersionImpl() const
{
    assert(m_type != nullptr);
    return m_type->getSinceVersion();
}

bool DataField::parseImpl()
{
    auto& type = getType();
    if (type.empty()) {
        log::error() << "The field \"" << getName() << "\" doesn't specify its type." << std::endl;
        return false;
    }

    auto* typePtr = getDb().findType(type);
    if (typePtr == nullptr) {
        log::error() << "Type \"" << type << "\" required by field \"" << getName() << "\" hasn't been found." << std::endl;
        return false;
    }

    if (typePtr->getKind() != Type::Kind::Composite) {
        log::error() << "Type \"" << type << "\" references by field \"" << getName() << "\" is expected to be composite." << std::endl;
        return false;
    }

    auto* compType = asCompositeType(typePtr);
    if (!compType->isValidData()) {
        log::error() << "Composite \"" << type << "\" is not of right format to support encoding of data field \"" <<
                        getName() << "\"." << std::endl;
        return false;
    }

    compType->recordDataUse();
    m_type = typePtr;
    recordExtraHeader(common::localHeader(getDb().getProtocolNamespace(), common::fieldNamespaceNameStr(), m_type->getName() + ".h"));
    return true;
}

bool DataField::writeImpl(std::ostream& out, unsigned indent, const std::string& suffix)
{
    assert(m_type != nullptr);
    writeHeader(out, indent, suffix);
    std::string name = common::refName(getName(), suffix);

    auto* typeSuffixPtr = &common::emptyString();
    if (m_type->isCommsOptionalWrapped() && isCommsOptionalWrapped()) {
        typeSuffixPtr = &common::optFieldSuffixStr();
    }
    auto typeRefName = common::refName(m_type->getName(), *typeSuffixPtr);

    out << output::indent(indent) << "using " << name << " = \n" <<
           output::indent(indent + 1) << common::fieldNamespaceStr() << typeRefName << "<\n";
    auto extraOpts = m_type->getExtraOptInfos();
    for (auto& o : extraOpts) {
        out << output::indent(indent + 2) << common::optParamPrefixStr();
        if (!ba::starts_with(o.second, common::fieldNamespaceStr())) {
            out << common::fieldNamespaceStr();
        }
        out << o.second << ",\n";
    }
    out << output::indent(indent + 2) << getFieldOptString() << '\n' <<
           output::indent(indent + 1) << ">;\n\n";
    return true;
}

bool DataField::usesBuiltInTypeImpl() const
{
    return false;
}

bool DataField::writePluginPropertiesImpl(
    std::ostream& out,
    unsigned indent,
    const std::string& scope,
    bool returnResult,
    bool commsOptionalWrapped)
{
    std::string props;
    common::scopeToPropertyDefNames(scope, getName(), commsOptionalWrapped, nullptr, &props);

    std::string typePropsStr =
        common::pluginNamespaceStr() +
        common::fieldNamespaceStr() +
        "createProps_" + m_type->getName() + "(\"" + getName() + "\")";

    if (commsOptionalWrapped && m_type->isCommsOptionalWrapped()) {
        typePropsStr = "comms_champion::property::field::Optional(" + typePropsStr + ").field()";
    }

    out << output::indent(indent) << "auto " << props << " =\n" <<
           output::indent(indent + 1) << typePropsStr << ";\n\n";

    if (returnResult) {
        out << output::indent(indent) << "return " << props << ";\n";
    }

    return true;
}

} // namespace sbe2comms
