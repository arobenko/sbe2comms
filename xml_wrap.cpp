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

#include "xml_wrap.h"

#include <cassert>

namespace sbe2comms
{

XmlPropsMap xmlParseNodeProps(xmlNodePtr node, xmlDocPtr doc)
{
    assert(node != nullptr);
    XmlPropsMap map;
    auto* prop = node->properties;
    while (prop != nullptr) {
        XmlCharPtr valuePtr(xmlNodeListGetString(doc, prop->children, 1));
        map.insert(
            std::make_pair(
                reinterpret_cast<const char*>(prop->name),
                reinterpret_cast<const char*>(valuePtr.get())));
        prop = prop->next;
    }
    return map;
}

std::string xmlText(xmlNodePtr node)
{
    assert(node != nullptr);
    auto* child = node->children;
    while (child != nullptr) {
        if (child->type == XML_TEXT_NODE) {
            XmlCharPtr valuePtr(xmlNodeGetContent(child));
            return std::string(reinterpret_cast<const char*>(valuePtr.get()));
        }
        child = child->next;
    }
    return std::string();
}

std::list<xmlNodePtr> xmlChildren(xmlNodePtr node, const std::string& name)
{
    std::list<xmlNodePtr> result;
    auto* cur = node->children;
    while (cur != nullptr) {
        do {
            if (cur->type != XML_ELEMENT_NODE) {
                break;
            }

            if (name.empty()) {
                result.push_back(cur);
                break;
            }

            std::string elemName(reinterpret_cast<const char*>(cur->name));
            if (elemName == name) {
                result.push_back(cur);
                break;
            }
        } while (false);

        cur = cur->next;
    }
    return result;
}

} // namespace sbe2comms
