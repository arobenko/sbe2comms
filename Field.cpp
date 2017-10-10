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

#include "DB.h"
#include "output.h"
#include "prop.h"
#include "common.h"
#include "BasicField.h"
#include "GroupField.h"
#include "DataField.h"
#include "log.h"

namespace sbe2comms
{

bool Field::parse()
{
    m_props = xmlParseNodeProps(m_node, m_db.getDoc());
    if ((m_props.empty()) || (getName().empty())) {
        log::error() << "Unexpected field properties" << std::endl;
        return false;
    }

    unsigned deprecated = prop::deprecated(m_props);
    unsigned sinceVer = prop::sinceVersion(m_props);
    if (deprecated <= sinceVer) {
        log::error() << "The field \"" << getName() << "\" has been deprecated before introduced." << std::endl;
        return false;
    }
    return parseImpl();
}

bool Field::doesExist() const
{
    return m_db.doesElementExist(prop::sinceVersion(m_props), prop::deprecated(m_props));
}

const std::string& Field::getName() const
{
    assert(!m_props.empty());
    return prop::name(m_props);
}

const std::string& Field::getDescription() const
{
    assert(!m_props.empty());
    return prop::description(m_props);
}

Field::Ptr Field::create(DB& db, xmlNodePtr node, const std::string& msgName)
{
    using FieldCreateFunc = std::function<Ptr (DB&, xmlNodePtr, const std::string&)>;
    static const std::map<std::string, FieldCreateFunc> CreateMap = {
        std::make_pair(
            "field",
            [](DB& d, xmlNodePtr n, const std::string& msg)
            {
                return Ptr(new BasicField(d, n, msg));
            }),
        std::make_pair(
            "group",
            [](DB& d, xmlNodePtr n, const std::string& msg)
            {
                return Ptr(new GroupField(d, n, msg));
            }),
        std::make_pair(
            "data",
            [](DB& d, xmlNodePtr n, const std::string& msg)
            {
                return Ptr(new DataField(d, n, msg));
            })
    };

    std::string kind(reinterpret_cast<const char*>(node->name));
    auto iter = CreateMap.find(kind);
    if (iter == CreateMap.end()) {
        return Ptr();
    }

    return iter->second(db, node, msgName);
}

bool Field::write(std::ostream& out, unsigned indent)
{
    auto& optMode = getDefaultOptMode();
    if (optMode.empty()) {
        return writeImpl(out, indent, common::emptyString());
    }

    static const std::string OptFieldSuffix("Field");
    bool result = writeImpl(out, indent, OptFieldSuffix);
    if (!result) {
        return false;
    }

    auto extraOpts = hasListOrString();
    writeBrief(out, indent, common::emptyString(), extraOpts);
    if (extraOpts) {
        writeOptions(out, indent);
    }

    out << output::indent(indent) << "using " << getName() << " =\n" <<
           output::indent(indent + 1) << "comms::field::Optional<\n" <<
           output::indent(indent + 2) << getName() << OptFieldSuffix;
    if (extraOpts) {
        out << "<TOpt...>";
    }
    out << ",\n" <<
           output::indent(indent + 2) << "comms::option::DefaultOptionalMode<" << optMode << ">\n" <<
           output::indent(indent + 1) << ">;\n\n";
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

unsigned Field::getSinceVersion() const
{
    assert(!m_props.empty());
    return prop::sinceVersion(m_props);
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

void Field::updateExtraHeaders(std::set<std::string>& headers)
{
    for (auto& h : m_extraHeaders) {
        if (headers.find(h) != headers.end()) {
            continue;
        }

        headers.insert(h);
    }
}

bool Field::parseImpl()
{
    return true;
}

bool Field::hasListOrStringImpl() const
{
    return false;
}

bool Field::writeBrief(std::ostream& out, unsigned indent, const std::string& suffix, bool extraOpts)
{
    auto& name = getName();
    assert(!name.empty());

    if (suffix.empty()) {
        out << output::indent(indent) << "/// \\brief Definition of \"" << name << "\" field.\n";
    }
    else {
        out << output::indent(indent) << "/// \\brief Definition of inner field of the optional \\ref " << name << " field.\n";
    }

    auto& desc = getDescription();
    if (!desc.empty()) {
        out << output::indent(indent) << "/// \\details " << desc << '\n';
    }

    if (extraOpts) {
        out << output::indent(indent) << "/// \\tparam TOpt Extra options from \\b comms::option namespace.\n";
    }

    return true;
}

void Field::writeOptions(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "template <typename... TOpt>\n";
}

void Field::recordExtraHeader(const std::string& header)
{
    auto iter = m_extraHeaders.find(header);
    if (iter != m_extraHeaders.end()) {
        return;
    }
    m_extraHeaders.insert(header);
}

const std::string& Field::getDefaultOptMode()
{
    if (getDeprecated() <= m_db.getSchemaVersion()) {
        static const std::string Mode("comms::field::OptionalMode::Missing");
        return Mode;
    }

    if (m_db.getMinRemoteVersion() < getSinceVersion()) {
        static const std::string Mode("comms::field::OptionalMode::Exists");
        return Mode;
    }

    return common::emptyString();
}

} // namespace sbe2comms
