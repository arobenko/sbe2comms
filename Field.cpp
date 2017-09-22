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
#include "get.h"
#include "BasicField.h"
#include "GroupField.h"
#include "DataField.h"

namespace sbe2comms
{

bool Field::write(std::ostream& out, DB& db, unsigned indent)
{
//    if ((isDeperated(db)) || (!isIntroduced(db))) {
//        // Don't write anything if type is deprecated or was introduced in later version
//        return true;
//    }

    return writeImpl(out, db, indent);
}

const XmlPropsMap& Field::props(DB& db)
{
    if (!m_props.empty()) {
        return m_props;
    }

    m_props = xmlParseNodeProps(m_node, db.getDoc());
    return m_props;
}

Field::Ptr Field::create(xmlNodePtr node, const std::string& msgName)
{
    using FieldCreateFunc = std::function<Ptr (xmlNodePtr, const std::string&)>;
    static const std::map<std::string, FieldCreateFunc> CreateMap = {
        std::make_pair(
            "field",
            [](xmlNodePtr n, const std::string& msg)
            {
                return Ptr(new BasicField(n, msg));
            }),
        std::make_pair(
            "group",
            [](xmlNodePtr n, const std::string& msg)
            {
                return Ptr(new GroupField(n, msg));
            }),
        std::make_pair(
            "data",
            [](xmlNodePtr n, const std::string& msg)
            {
                return Ptr(new DataField(n, msg));
            })
    };

    std::string kind(reinterpret_cast<const char*>(node->name));
    auto iter = CreateMap.find(kind);
    if (iter == CreateMap.end()) {
        return Ptr();
    }

    return iter->second(node, msgName);
}

bool Field::startWrite(std::ostream& out, DB& db, unsigned indent)
{
    auto& p = props(db);
    auto& name = prop::name(p);
    if (name.empty()) {
        std::cerr << output::indent(1) <<
            "ERROR: Field doesn't provide name." << std::endl;
        return false;
    }

    out << output::indent(indent) << "/// \\brief Definition of \"" << name << "\" field.\n";
    auto& desc = prop::description(p);
    if (!desc.empty()) {
        out << output::indent(indent) << "/// \\details " << desc << '\n';
    }

    return true;
}

std::string Field::extraOptionsString(DB& db)
{
    auto& name = prop::name(props(db));
    return "details::" + m_msgName + "Fields::ExtraOptionsFor_" + name + "<TOpt>";
}

} // namespace sbe2comms
