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
#include <functional>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "BasicType.h"
#include "CompositeType.h"
#include "EnumType.h"
#include "SetType.h"
#include "prop.h"
#include "get.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{

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

bool DB::parseSchema(std::string filename)
{
    m_doc.reset(xmlParseFile(filename.c_str()));
    if (!m_doc) {
        std::cerr << "ERROR: Invalid schema file: \"" << filename << "\"!" << std::endl;
        return false;
    }

    auto* root = xmlDocGetRootElement(m_doc.get());
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

    m_messageSchema.reset(new MessageSchema(root, m_doc.get()));

    using ParseChildNodeFunc = bool (DB::*)(xmlNodePtr node);

    std::map<std::string, ParseChildNodeFunc> parseFuncMap = {
        std::make_pair("types", &DB::parseTypes),
        std::make_pair("message", &DB::parseMessage)
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
            if (!((this->*func)(cur))) {
                return false;
            }
        } while (false);

        cur = cur->next;
    }

    return true;
}

const std::string& DB::getRootPath()
{
    if (m_cache.m_rootDir.empty()) {
        // TODO: program options
        m_cache.m_rootDir = bf::current_path().string();
    }

    assert(!m_cache.m_rootDir.empty());
    return m_cache.m_rootDir;
}

const std::string& DB::getProtocolNamespace()
{
    if (m_cache.m_namespace) {
        return *m_cache.m_namespace;
    }

    assert(m_messageSchema);
    auto package = m_messageSchema->package();
    ba::replace_all(package, " ", "_");
    m_cache.m_namespace = std::move(package);
    return *m_cache.m_namespace;
}

const std::string& DB::getProtocolRelDir()
{
    if (m_cache.m_protocolRelDir.empty()) {
        bf::path path(get::includeDirName());
        auto& ns = getProtocolNamespace();
        if (!ns.empty()) {
            path /= ns;
        }

        m_cache.m_protocolRelDir = path.string();
    }

    assert(!m_cache.m_protocolRelDir.empty());
    return m_cache.m_protocolRelDir;
}

unsigned DB::getSchemaVersion()
{
    auto& val = m_cache.m_schemaVersion;
    if (!val) {
        // TODO: check options for override
        assert(m_messageSchema);
        val = m_messageSchema->version();
    }

    return *val;
}

const std::string& DB::getEndian()
{
    auto& val = m_cache.m_endian;
    if (!val.empty()) {
        return val;
    }

    assert(m_messageSchema);
    auto& byteOrder = m_messageSchema->byteOrder();

    static const std::string BigEndianStr("bigEndian");
    if (byteOrder == BigEndianStr) {
        val = "comms::option::BigEndian";
        return val;
    }

    val = "comms::option::LittleEndian";
    return val;
}

bool DB::recordTypeRef(xmlNodePtr node)
{
    auto props = xmlParseNodeProps(node, m_doc.get());
    auto& name = prop::name(props);

    if (name.empty()) {
        std::cerr << "ERROR: type element \"" <<
            node->name << "\" does NOT have name property" << std::endl;
        return false;
    }

    auto iter = m_types.find(name);
    if (iter != m_types.end()) {
        std::cerr << "ERROR: type \"" << name << "\" has been defined more than once" << std::endl;
        return false;
    }

    std::string kind(reinterpret_cast<const char*>(node->name));
    auto ptr = Type::create(kind, *this, node);
    if (!ptr) {
        return false;
    }

    if (!ptr->parse()) {
        return false;
    }

    m_types.insert(std::make_pair(name, std::move(ptr)));
    return true;
}

bool DB::parseTypes(xmlNodePtr node)
{
    auto* cur = node->children;
    while (cur != nullptr) {
        if (cur->type == XML_ELEMENT_NODE) {
            if (!recordTypeRef(cur)) {
                return false;
            }
        }

        cur = cur->next;
    }
    return true;
}

bool DB::parseMessage(xmlNodePtr node)
{
    auto props = xmlParseNodeProps(node, m_doc.get());
    auto& name = prop::name(props);
    if (name.empty()) {
        std::cerr << "ERROR: message element does NOT have name property" << std::endl;
        return false;
    }

    auto iter = m_messages.find(name);
    if (iter != m_messages.end()) {
        std::cerr << "ERROR: message \"" << name << "\" has been defined more than once" << std::endl;
        return false;
    }

    Message msg(*this, node);
    if (!msg.parse()) {
        return false;
    }

    m_messages.insert(std::make_pair(name, std::move(msg)));
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
