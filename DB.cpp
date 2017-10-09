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
#include "log.h"

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
        log::error() << "Invalid schema file: \"" << filename << "\"!" << std::endl;
        return false;
    }

    auto* root = xmlDocGetRootElement(m_doc.get());
    if (root == nullptr) {
        log::error() << "Failed to fine root element in the schema!" << std::endl;
        return false;
    }

    static const std::string SchemaName("messageSchema");
    std::string rootName(reinterpret_cast<const char*>(root->name));
    std::size_t pos = rootName.find(SchemaName);
    if ((pos == std::string::npos) || (pos != (rootName.size() - SchemaName.size()))) {
        log::error() << "Root element is not " << SchemaName;
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
                log::warning() << "Unexpected element: \"" << elemName << "\", ignored..." << std::endl;
                break;
            }

            auto func = iter->second;
            if (!((this->*func)(cur))) {
                return false;
            }
        } while (false);

        cur = cur->next;
    }

    for (auto& t : m_types) {
        if (!t.second->parse()) {
            return false;
        }
    }

    for (auto& m : m_messages) {
        if (!m.second.parse()) {
            return false;
        }
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

unsigned DB::getMinRemoteVersion()
{
    return 0U;
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

bool DB::doesElementExist(unsigned introducedSince, unsigned deprecatedSince)
{
    return
        ((introducedSince <= getSchemaVersion()) &&
         (getMinRemoteVersion() < deprecatedSince));
}

const Type* DB::findType(const std::string& name) const
{
    auto iter = m_types.find(name);
    if (iter == m_types.end()) {
        return nullptr;
    }

    assert(iter->second);
    return iter->second.get();
}

Type* DB::findType(const std::string& name)
{
    return const_cast<Type*>(static_cast<const DB*>(this)->findType(name));
}

const Type* DB::getBuiltInType(const std::string& name)
{
    auto iter = m_builtInTypes.find(name);
    if (iter != m_builtInTypes.end()) {
        assert(iter->second.m_type);
        return iter->second.m_type.get();
    }

    static const std::set<std::string> BuiltIns = {
        "int8",
        "uint8",
        "int16",
        "uint16",
        "int32",
        "uint32",
        "int64",
        "uint64",
        "float",
        "double"
    };

    auto setIter = BuiltIns.find(name);
    if (setIter == BuiltIns.end()) {
        return nullptr;
    }

    BuiltInTypeInfo info;
    info.m_node = xmlCreateBuiltInType(name);
    info.m_type = Type::create(*this, info.m_node.get());
    assert(info.m_type);

    if (!info.m_type->parse()) {
        assert(!"Failed to parse builtIn type");
        return nullptr;
    }

    auto* result = info.m_type.get();
    m_builtInTypes.insert(std::make_pair(name, std::move(info)));
    return result;
}

bool DB::isRecordedBuiltInType(const std::string& name) const
{
    return m_builtInTypes.find(name) != m_builtInTypes.end();
}

bool DB::recordTypeRef(xmlNodePtr node)
{
    auto props = xmlParseNodeProps(node, m_doc.get());
    auto& name = prop::name(props);

    if (name.empty()) {
        log::error() << "type element \"" <<
            node->name << "\" does NOT have name property" << std::endl;
        return false;
    }

    auto iter = m_types.find(name);
    if (iter != m_types.end()) {
        log::error() << "type \"" << name << "\" has been defined more than once" << std::endl;
        return false;
    }

    auto ptr = Type::create(*this, node);
    if (!ptr) {
        return false;
    }

    if (doesElementExist(prop::sinceVersion(props), prop::deprecated(props))) {
        m_types.insert(std::make_pair(name, std::move(ptr)));
    }

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
        log::error() << "message element does NOT have name property" << std::endl;
        return false;
    }

    auto iter = m_messages.find(name);
    if (iter != m_messages.end()) {
        log::error() << "message \"" << name << "\" has been defined more than once" << std::endl;
        return false;
    }

    Message msg(*this, node);
    if (doesElementExist(prop::sinceVersion(props), prop::deprecated(props))) {
        m_messages.insert(std::make_pair(name, std::move(msg)));
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
