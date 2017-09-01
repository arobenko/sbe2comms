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

#include "DB.h"

#include <iostream>
#include <cassert>

namespace sbe2comms
{

namespace
{

const std::string NameStr("name");

bool recordTypeRef(xmlNodePtr node, DB& db)
{
    std::string name;
    auto* prop = node->properties;
    while (prop != nullptr) {
        auto* propName = reinterpret_cast<const char*>(prop->name);
        if (propName == NameStr) {
            XmlCharPtr valuePtr(xmlNodeListGetString(db.m_doc.get(), prop->children, 1));
            name = reinterpret_cast<const char*>(valuePtr.get());
            break;
        }
        prop = prop->next;
    }

    if (name.empty()) {
        std::cerr << "ERROR: type element \"" <<
            node->name << "\" does NOT have name property" << std::endl;
        return false;
    }

    auto iter = db.m_types.find(name);
    if (iter != db.m_types.end()) {
        std::cerr << "ERROR: type \"" << name << "\" has been defined more than once" << std::endl;
        return false;
    }

    db.m_types.insert(std::make_pair(std::move(name), Type(node)));
    return true;
}

bool parseTypes(xmlNodePtr node, DB& db)
{
    auto* cur = node->children;
    while (cur != nullptr) {
        if (cur->type == XML_ELEMENT_NODE) {
            if (!recordTypeRef(cur, db)) {
                return false;
            }
        }

        cur = cur->next;
    }
    return true;
}

bool parseMessage(xmlNodePtr node, DB& db)
{
    std::string name;
    auto* prop = node->properties;
    while (prop != nullptr) {
        auto* propName = reinterpret_cast<const char*>(prop->name);
        if (propName == NameStr) {
            XmlCharPtr valuePtr(xmlNodeListGetString(db.m_doc.get(), prop->children, 1));
            name = reinterpret_cast<const char*>(valuePtr.get());
            break;
        }
        prop = prop->next;
    }

    if (name.empty()) {
        std::cerr << "ERROR: message element does NOT have name property" << std::endl;
        return false;
    }

    auto iter = db.m_messages.find(name);
    if (iter != db.m_messages.end()) {
        std::cerr << "ERROR: message \"" << name << "\" has been defined more than once" << std::endl;
        return false;
    }

    db.m_messages.insert(std::make_pair(std::move(name), Message(node)));
    return true;
}

//bool updateConfigNamespace(const PropsMap& map, DB& db)
//{
//    const std::string PackageProp("package");
//    auto iter = map.find(PackageProp);
//    if (iter == map.end()) {
//        db.m_config.m_namespace.clear();
//        return true;
//    }

//    auto& name = iter->second;
//    // TODO: remove spaces
//    db.m_config.m_namespace = name;
//    return true;
//}

//bool updateConfigEndian(const PropsMap& map, DB& db)
//{
//    const std::string EndianProp("byteOrder");
////    const std::string LittleEndian("comms::option::LittleEndian");
////    const std::string BigEndian("comms::option::BigEndian");
//    static_cast<void>(db);
//    auto iter = map.find(EndianProp);
//    if (iter == map.end()) {
//        return false;
//    }
//    return true;
//}

} // namespace

bool parseSchema(std::string filename, DB& db)
{
    db.m_doc.reset(xmlParseFile(filename.c_str()));
    if (!db.m_doc) {
        std::cerr << "ERROR: Invalid schema file: \"" << filename << "\"!" << std::endl;
        return false;
    }

    auto* root = xmlDocGetRootElement(db.m_doc.get());
    if (root == nullptr) {
        std::cerr << "ERROR: Failed to fine root element in the schema!" << std::endl;
        return false;
    }

    static const std::string SchemaName("messageSchema");
    std::string rootName(reinterpret_cast<const char*>(root->name));
    std::size_t pos = rootName.find(SchemaName);
    if ((pos == std::string::npos) || (pos != (rootName.size() - SchemaName.size()))) {
        std::cerr << "ERROR: Root element is not " << SchemaName;
        return false;
    }

    db.m_messageSchema.reset(new MessageSchema(root, db.m_doc.get()));

    using ParseChildNodeFunc = bool (*)(xmlNodePtr node, DB& db);

    std::map<std::string, ParseChildNodeFunc> parseFuncMap = {
        std::make_pair("types", static_cast<ParseChildNodeFunc>(&parseTypes)),
        std::make_pair("message", static_cast<ParseChildNodeFunc>(&parseMessage))
    };

    auto* cur = root->children;
    while (cur != nullptr) {
        do {
            if (cur->type != XML_ELEMENT_NODE) {
                break;
            }

            std::string elemName(reinterpret_cast<const char*>(cur->name));
            auto iter = parseFuncMap.find(elemName);
            if (iter == parseFuncMap.end()) {
                std::cerr << "WARNING: Unexpected element: \"" << elemName << "\", ignored..." << std::endl;
                break;
            }

            auto func = iter->second;
            if (!func(cur, db)) {
                return false;
            }
        } while (false);

        cur = cur->next;
    }

    return true;
}

//bool updateConfig(DB& db)
//{
//    auto props = parseNodeProps(db.m_schema.m_messageSchema, db);
//    return
//        updateConfigNamespace(props, db) &&
//        updateConfigEndian(props, db);
//}

} // namespace sbe2comms
