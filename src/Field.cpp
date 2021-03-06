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

#include "Field.h"

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "output.h"
#include "prop.h"
#include "common.h"
#include "BasicField.h"
#include "GroupField.h"
#include "DataField.h"
#include "log.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

bool Field::parse()
{
    m_props = xmlParseNodeProps(m_node, m_db.getDoc());
    if ((m_props.empty()) || (getName().empty())) {
        log::error() << "Unexpected field properties" << std::endl;
        return false;
    }

    if (!parseImpl()) {
        return false;
    }

    if (!getDefaultOptMode().empty()) {
        recordExtraHeader("\"comms/field/Optional.h\"");
    }

    unsigned deprecated = prop::deprecated(m_props);
    unsigned sinceVer = prop::sinceVersion(m_props);
    if (deprecated <= sinceVer) {
        log::warning() << "The field \"" << getName() << "\" has been deprecated before introduced." << std::endl;
    }

    auto refTypeSinceVersion = getReferencedTypeSinceVersionImpl();
    if (sinceVer < refTypeSinceVersion) {
        log::error() << "The field \"" << getName() << "\" references type that has been introduced later." << std::endl;
        return false;
    }

    return true;
}

bool Field::doesExist() const
{
    return m_db.doesElementExist(prop::sinceVersion(m_props));
}

const std::string& Field::getName() const
{
    assert(!m_props.empty());
    return prop::name(m_props);
}

const std::string& Field::getReferenceName() const
{
    return common::renameKeyword(getName());
}

const std::string& Field::getDescription() const
{
    assert(!m_props.empty());
    return prop::description(m_props);
}

Field::Ptr Field::create(DB& db, xmlNodePtr node, const std::string& scope)
{
    using FieldCreateFunc = std::function<Ptr (DB&, xmlNodePtr, const std::string&)>;
    static const std::map<std::string, FieldCreateFunc> CreateMap = {
        std::make_pair(
            "field",
            [](DB& d, xmlNodePtr n, const std::string& s)
            {
                return Ptr(new BasicField(d, n, s));
            }),
        std::make_pair(
            "group",
            [](DB& d, xmlNodePtr n, const std::string& s)
            {
                return Ptr(new GroupField(d, n, s));
            }),
        std::make_pair(
            "data",
            [](DB& d, xmlNodePtr n, const std::string& s)
            {
                return Ptr(new DataField(d, n, s));
            })
    };

    std::string kind(reinterpret_cast<const char*>(node->name));
    auto iter = CreateMap.find(kind);
    if (iter == CreateMap.end()) {
        return Ptr();
    }

    return iter->second(db, node, scope);
}

bool Field::write(std::ostream& out, unsigned indent)
{
    auto& optMode = getDefaultOptMode();
    if (optMode.empty()) {
        return writeImpl(out, indent, common::emptyString());
    }

    bool result = writeImpl(out, indent, common::optFieldSuffixStr());
    if (!result) {
        return false;
    }

    writeHeader(out, indent, common::emptyString());
    out << output::indent(indent) << "struct " << getReferenceName() << " : public\n" <<
           output::indent(indent + 1) << "comms::field::Optional<\n" <<
           output::indent(indent + 2) << getName() << common::optFieldSuffixStr() << ",\n" <<
           output::indent(indent + 2) << "comms::option::DefaultOptionalMode<" << optMode << ">\n" <<
           output::indent(indent + 1) << ">\n";

    common::writeOptFieldDefinitionBody(out, indent, getSinceVersion());
    return true;
}

bool Field::writePluginProperties(
    std::ostream& out,
    unsigned indent,
    const std::string& scope,
    bool returnResult)
{
    bool commsOptionalWrapped = isCommsOptionalWrapped();
    bool wrapProps = commsOptionalWrapped;
    do {
        if (!commsOptionalWrapped) {
            break;
        }

        if (isForcedCommsOptionalImpl()) {
            wrapProps = true;
            break;
        }

        wrapProps = (getReferencedTypeSinceVersionImpl() <= m_db.getMinRemoteVersion());
    } while (false);

    bool fieldsReturnResult = returnResult && (!wrapProps);

    if (!writePluginPropertiesImpl(out, indent, scope, fieldsReturnResult, wrapProps)) {
        return false;
    }

    if (!wrapProps) {
        return true;
    }

    assert(commsOptionalWrapped);
    std::string fieldProps;
    common::scopeToPropertyDefNames(scope, getName(), true, nullptr, &fieldProps);

    std::string type;
    std::string props;
    common::scopeToPropertyDefNames(scope, getName(), false, &type, &props);

    auto nameStr = '\"' + getName() + '\"';

    out << output::indent(indent) << "using " << type << " = " <<
           scope + getReferenceName() <<
           ";\n" <<
           output::indent(indent) << "auto " << props << " = \n" <<
           output::indent(indent + 1) << "comms_champion::property::field::ForField<" << type << ">()\n" <<
           output::indent(indent + 2) << ".name(" << nameStr << ")\n" <<
           output::indent(indent + 2) << ".uncheckable()\n" <<
           output::indent(indent + 2) << ".field(" << fieldProps << ")\n" <<
           output::indent(indent + 2) << ".asMap();\n\n";

    if (returnResult) {
        out << output::indent(indent) << "return " << props << ";\n";
    }
    return true;
}

bool Field::hasPresence() const
{
    assert(!m_props.empty());
    return !prop::presence(m_props).empty();
}

bool Field::isRequired() const
{
    assert(!m_props.empty());
    return prop::isRequired(m_props);
}

bool Field::isOptional() const
{
    assert(!m_props.empty());
    return prop::isOptional(m_props);
}

bool Field::isConstant() const
{
    assert(!m_props.empty());
    return prop::isConstant(m_props);
}

unsigned Field::getDeprecated() const
{
    assert(!m_props.empty());
    return prop::deprecated(m_props);
}

const std::string& Field::getType() const
{
    assert(!m_props.empty());
    return prop::type(m_props);
}

unsigned Field::getOffset() const
{
    assert(!m_props.empty());
    return prop::offset(m_props);
}

void Field::updateExtraHeaders(ExtraHeaders& headers)
{
    for (auto& h : m_extraHeaders) {
        common::recordExtraHeader(h, headers);
    }
}

bool Field::isCommsOptionalWrapped() const
{
    return !getDefaultOptMode().empty();
}

unsigned Field::getSinceVersionImpl() const
{
    assert(!m_props.empty());
    return prop::sinceVersion(m_props);
}

unsigned Field::getReferencedTypeSinceVersionImpl() const
{
    return 0U;
}

bool Field::isForcedCommsOptionalImpl() const
{
    return false;
}

bool Field::parseImpl()
{
    return true;
}

bool Field::writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope)
{
    out << output::indent(indent) << "/// \\brief Default options for \\ref " << scope << getReferenceName() << " field.\n" <<
           output::indent(indent) << "using " << getReferenceName() << common::eqEmptyOptionStr() << ";\n\n";
    return true;
}

void Field::writeHeader(std::ostream& out, unsigned indent, const std::string& suffix)
{
    if (suffix.empty()) {
        out << output::indent(indent) << "/// \\brief Definition of \"" << getName() << "\" field.\n";
    }
    else {
        out << output::indent(indent) << "/// \\brief Definition of inner field of the optional \\ref " << getReferenceName() << " field.\n";
    }

    common::writeDetails(out, indent, getDescription());
}

void Field::writeOptions(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "template <typename... TOpt>\n";
}

void Field::recordExtraHeader(const std::string& header)
{
    common::recordExtraHeader(header, m_extraHeaders);
}

void Field::recordMultipleExtraHeaders(const ExtraHeaders& headers)
{
    for (auto& h : headers) {
        recordExtraHeader(h);
    }
}

std::string Field::getFieldOptString() const
{
    return common::optParamPrefixStr() + common::messageNamespaceStr() +
           getScope() +
           getReferenceName();
}

std::string Field::getTypeOptString(const Type& type) const
{
    auto typeOpts = type.getExtraOptInfos();
    assert(typeOpts.size() == 1U);
    std::string result = common::optParamPrefixStr();
    if (!ba::starts_with(getScope(), common::fieldNamespaceStr())) {
        result += common::fieldNamespaceStr();
    }
    result += typeOpts.front().second;
    return result;
}

const std::string& Field::getCreatePropsCallSuffix() const
{
    if (!m_inGroup) {
        return common::emptyString();
    }

    static const std::string Str(", true");
    return Str;
}

void Field::scopeToPropertyDefNames(
    const std::string& scope,
    std::string* fieldType,
    std::string* propsName)
{
    return common::scopeToPropertyDefNames(scope, getName(), isCommsOptionalWrapped(), fieldType, propsName);
}

const std::string& Field::getDefaultOptMode() const
{
    auto sinceVersion = getSinceVersion();
    if (m_containingGroupVersion == sinceVersion) {
        return common::emptyString();
    }

    if (sinceVersion <= m_db.getMinRemoteVersion()) {
        return common::emptyString();
    }

    auto refTypeSinceVersion = getReferencedTypeSinceVersionImpl();
    if ((sinceVersion <= refTypeSinceVersion) && (!isForcedCommsOptionalImpl())) {
        return common::emptyString();
    }

    static const std::string Mode("comms::field::OptionalMode::Exists");
    return Mode;
}

} // namespace sbe2comms
