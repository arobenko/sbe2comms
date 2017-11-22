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
#include <string>

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

XmlNodePtr xmlCreatePadding(unsigned idx, unsigned len)
{
    return xmlCreateRawDataType("pad" + std::to_string(idx) + '_', len);
}

XmlNodePtr xmlCreateRawDataType(const std::string& name, unsigned len)
{
    static const std::string type("type");
    auto* typePtr = reinterpret_cast<const xmlChar*>(type.c_str());
    XmlNodePtr ptr(xmlNewNode(nullptr, typePtr));

    static const std::string nameStr("name");
    auto* namePtr = reinterpret_cast<const xmlChar*>(nameStr.c_str());
    auto* nameValPtr = reinterpret_cast<const xmlChar*>(name.c_str());
    xmlNewProp(ptr.get(), namePtr, nameValPtr);

    static const std::string lengthStr("length");
    auto* lengthPtr = reinterpret_cast<const xmlChar*>(lengthStr.c_str());
    auto lengthVal = std::to_string(len);
    auto* lengthValPtr = reinterpret_cast<const xmlChar*>(lengthVal.c_str());
    xmlNewProp(ptr.get(), lengthPtr, lengthValPtr);

    static const std::string primTypeStr("primitiveType");
    auto* primTypePtr = reinterpret_cast<const xmlChar*>(primTypeStr.c_str());
    static const std::string primTypeVal("uint8");
    auto* primTypeValPtr = reinterpret_cast<const xmlChar*>(primTypeVal.c_str());
    xmlNewProp(ptr.get(), primTypePtr, primTypeValPtr);
    return ptr;
}

XmlNodePtr xmlCreateBuiltInType(const std::string& name)
{
    static const std::string type("type");
    auto* typePtr = reinterpret_cast<const xmlChar*>(type.c_str());
    XmlNodePtr ptr(xmlNewNode(nullptr, typePtr));

    static const std::string nameStr("name");
    auto* namePtr = reinterpret_cast<const xmlChar*>(nameStr.c_str());
    auto* nameValPtr = reinterpret_cast<const xmlChar*>(name.c_str());
    xmlNewProp(ptr.get(), namePtr, nameValPtr);

    static const std::string primTypeStr("primitiveType");
    auto* primTypePtr = reinterpret_cast<const xmlChar*>(primTypeStr.c_str());
    auto* primTypeValPtr = reinterpret_cast<const xmlChar*>(name.c_str());
    xmlNewProp(ptr.get(), primTypePtr, primTypeValPtr);
    return ptr;
}

XmlNodePtr xmlCreatePaddingField(unsigned idx, const std::string& typeName, unsigned sinceVersion)
{
    static const std::string fieldStr("field");
    auto* fieldStrPtr = reinterpret_cast<const xmlChar*>(fieldStr.c_str());
    XmlNodePtr ptr(xmlNewNode(nullptr, fieldStrPtr));

    static const std::string nameStr("name");
    auto* namePtr = reinterpret_cast<const xmlChar*>(nameStr.c_str());
    std::string nameVal("pad" + std::to_string(idx) + '_');
    auto* nameValPtr = reinterpret_cast<const xmlChar*>(nameVal.c_str());
    xmlNewProp(ptr.get(), namePtr, nameValPtr);

    static const std::string typeStr("type");
    auto* typePtr = reinterpret_cast<const xmlChar*>(typeStr.c_str());
    auto* typeValPtr = reinterpret_cast<const xmlChar*>(typeName.c_str());
    xmlNewProp(ptr.get(), typePtr, typeValPtr);

    if (sinceVersion != 0) {
        static const std::string sinceVersionStr("sinceVersion");
        auto sinceVersionPtr = reinterpret_cast<const xmlChar*>(sinceVersionStr.c_str());
        auto sinceVersionValStr = std::to_string(sinceVersion);
        auto sinceVersionValPtr = reinterpret_cast<const xmlChar*>(sinceVersionValStr.c_str());
        xmlNewProp(ptr.get(), sinceVersionPtr, sinceVersionValPtr);
    }
    return ptr;
}

XmlNodePtr xmlEnumValidValue(
    const std::string& name,
    const std::string& encType,
    const XmlEnumValuesList& values)
{
    static const std::string enumStr("enum");
    auto* enumStrPtr = reinterpret_cast<const xmlChar*>(enumStr.c_str());
    XmlNodePtr ptr(xmlNewNode(nullptr, enumStrPtr));

    static const std::string nameStr("name");
    auto* namePtr = reinterpret_cast<const xmlChar*>(nameStr.c_str());
    auto* nameValPtr = reinterpret_cast<const xmlChar*>(name.c_str());
    xmlNewProp(ptr.get(), namePtr, nameValPtr);

    static const std::string typeStr("encodingType");
    auto* typePtr = reinterpret_cast<const xmlChar*>(typeStr.c_str());
    auto* typeValPtr = reinterpret_cast<const xmlChar*>(encType.c_str());
    xmlNewProp(ptr.get(), typePtr, typeValPtr);

    for (auto& v : values) {
        static const std::string validValueStr("validValue");
        auto* validValueStrPtr = reinterpret_cast<const xmlChar*>(validValueStr.c_str());
        XmlNodePtr valuePtr(xmlNewNode(nullptr, validValueStrPtr));

        auto* valueNamePtr = reinterpret_cast<const xmlChar*>(v.first.c_str());
        xmlNewProp(valuePtr.get(), namePtr, valueNamePtr);

        auto* valueNumPtr = reinterpret_cast<const xmlChar*>(v.second.c_str());
        XmlNodePtr valueTextPtr(xmlNewText(valueNumPtr));
        auto result = xmlAddChild(valuePtr.get(), valueTextPtr.release());
        static_cast<void>(result);
        assert(result != nullptr);
        result = xmlAddChild(ptr.get(), valuePtr.release());
        static_cast<void>(result);
        assert(result != nullptr);
    }

    return ptr;
}


} // namespace sbe2comms
