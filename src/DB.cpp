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
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "BasicType.h"
#include "CompositeType.h"
#include "EnumType.h"
#include "SetType.h"
#include "prop.h"
#include "common.h"
#include "log.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

bool DB::parseSchema(const ProgramOptions& options)
{
    auto filename = options.getFile();
    if (filename.empty()) {
        sbe2comms::log::error() << "Message schema hasn't been provided." << std::endl;
        return false;
    }

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

    if (!processMessageSchema()) {
        return false;
    }

    if (!processOptions(options)) {
        return false;
    }

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

    log::info() << "Generating files in " << m_rootDir << std::endl;

    for (auto& t : m_types) {
        if (!t.second->parse()) {
            return false;
        }
    }

    for (auto& m : m_messages) {
        assert(m.second);
        if (!m.second->parse()) {
            return false;
        }
    }

    return true;
}

const std::string& DB::getRootPath() const
{
    assert(!m_rootDir.empty());
    return m_rootDir;
}

const std::string& DB::getProtocolNamespace() const
{
    return m_namespace;
}

unsigned DB::getSchemaVersion() const
{
    return m_schemaVersion;
}

unsigned DB::getSchemaId() const
{
    assert(m_messageSchema);
    return m_messageSchema->id();
}

const std::string& DB::getMessageHeaderType() const
{
    assert(m_messageSchema);
    return m_messageSchema->headerType();
}

unsigned DB::getMinRemoteVersion() const
{
    return m_minRemoteVersion;
}

const std::string& DB::getEndian() const
{
    return m_endian;
}

bool DB::doesElementExist(unsigned introducedSince) const
{
    return (introducedSince <= m_schemaVersion);
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

bool DB::isIntroducedType(const std::string& name) const
{
    return findType(name) != nullptr;
}

const Type* DB::getBuiltInType(const std::string& name)
{
    auto iter = m_builtInTypes.find(name);
    if (iter != m_builtInTypes.end()) {
        assert(iter->second.m_type);
        return iter->second.m_type.get();
    }

    static const std::set<std::string> BuiltIns = {
        "char",
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

    GeneratedTypeInfo info;
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

const Type* DB::getPaddingType(unsigned len)
{
    auto name = common::padStr() + std::to_string(len);
    auto iter = m_paddingTypes.find(name);
    if (iter != m_paddingTypes.end()) {
        assert(iter->second.m_type);
        return iter->second.m_type.get();
    }

    GeneratedTypeInfo info;
    info.m_node = xmlCreateRawDataType(name, len);
    info.m_type = Type::create(*this, info.m_node.get());
    assert(info.m_type);

    if (!info.m_type->parse()) {
        assert(!"Failed to parse padding type");
        return nullptr;
    }

    auto* result = info.m_type.get();
    m_paddingTypes.insert(std::make_pair(name, std::move(info)));
    return result;
}

const Type* DB::findPaddingType(const std::string& name) const
{
    auto iter = m_paddingTypes.find(name);
    if (iter != m_paddingTypes.end()) {
        assert(iter->second.m_type);
        return iter->second.m_type.get();
    }
    return nullptr;
}

bool DB::isRecordedPaddingType(const std::string& name) const
{
    return findPaddingType(name) != nullptr;
}

void DB::recordGroupListUsage()
{
    m_groupListUsed = true;
}

bool DB::isGroupListRecorded() const
{
    return m_groupListUsed;
}

bool DB::isPaddingRecorded() const
{
    return !m_paddingTypes.empty();
}

std::list<std::string> DB::getAllUsedBuiltInTypes() const
{
    std::list<std::string> list;
    for (auto& t : m_builtInTypes) {
        list.push_back(t.first);
    }
    return list;
}

xmlNodePtr DB::createMsgIdEnumNode(const std::string& name, const std::string& encType)
{
    assert(!m_msgIdEnum);
    XmlEnumValuesList values;
    values.reserve(m_messagesById.size());
    for (auto& m : m_messagesById) {
        values.push_back(
            std::make_pair(
                m.second->first,
                std::to_string(m.first)));
    }

    m_msgIdEnum = xmlEnumValidValue(name, encType, values);
    return m_msgIdEnum.get();
}

xmlNodePtr DB::getMsgIdEnumNode() const
{
    assert(m_msgIdEnum);
    return m_msgIdEnum.get();
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

    if (doesElementExist(prop::sinceVersion(props))) {
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

    if (!doesElementExist(prop::sinceVersion(props))) {
        return true;
    }

    std::unique_ptr<Message> msg(new Message(*this, node));
    auto id = prop::id(props);
    auto iterById = m_messagesById.find(id);
    if (iterById != m_messagesById.end()) {
        log::error() << "Message \"" << name << "\" doesn't have unique ID." << std::endl;
        return false;
    }

    auto insertIter = m_messages.insert(m_messages.end(), std::make_pair(name, std::move(msg)));
    assert(insertIter != m_messages.end());
    m_messagesById.insert(std::make_pair(id, insertIter));

    return true;
}

bool DB::processOptions(const ProgramOptions& options)
{
    return
        processOutputDirectory(options) &&
        processNamespace(options) &&
        processForcedSchemaVersion(options);
}

bool DB::processOutputDirectory(const ProgramOptions& options)
{
    auto dir = options.getOutputDirectory();
    if (dir.empty()) {
        m_rootDir = bf::current_path().string();
        return true;
    }

    bf::path dirPath(dir);
    if (dirPath.is_absolute()) {
        m_rootDir = dir;
        return true;
    }

    m_rootDir = (bf::current_path() / dirPath).string();
    return true;
}

bool DB::processNamespace(const ProgramOptions& options)
{
    if (options.hasNamespaceOverride()) {
        m_namespace = options.getNamespace();
    }
    else {
        assert(m_messageSchema);
        m_namespace = m_messageSchema->package();
    }

    ba::replace_all(m_namespace, " ", "_");
    return true;
}

bool DB::processForcedSchemaVersion(const ProgramOptions& options)
{
    assert(m_messageSchema);
    m_schemaVersion = m_messageSchema->version();
    if (!options.hasForcedSchemaVersion()) {
        return true;
    }

    auto newVer = options.getForcedSchemaVersion();
    if (m_schemaVersion < newVer) {
        log::error() << "Forced schema version is greater than specified in the schema file." << std::endl;
        return false;
    }

    if (newVer < m_schemaVersion) {
        log::info() << "Forcing schema version to " << newVer << std::endl;
    }

    m_schemaVersion = newVer;
    return true;
}

bool DB::processMinRemoteVersion(const ProgramOptions& options)
{
    m_minRemoteVersion = std::min(options.getMinRemoteVersion(), m_schemaVersion);
    return true;
}

bool DB::processMessageSchema()
{
    assert(m_messageSchema);
    auto& byteOrder = m_messageSchema->byteOrder();

    static const std::string BigEndianStr("bigEndian");
    if (byteOrder == BigEndianStr) {
        m_endian = "comms::option::BigEndian";
    }
    else {
        m_endian = "comms::option::LittleEndian";
    }
    return true;
}

} // namespace sbe2comms
