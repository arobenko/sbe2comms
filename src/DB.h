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

#pragma once

#include <map>
#include <list>
#include <string>

#include <boost/optional.hpp>

#include "xml_wrap.h"
#include "MessageSchema.h"
#include "Type.h"
#include "Message.h"
#include "ProgramOptions.h"

namespace sbe2comms
{

class DB
{
public:
    using TypesMap = std::map<std::string, TypePtr>;
    using MessagePtr = std::unique_ptr<Message>;
    using MessagesMap = std::map<std::string, MessagePtr>;
    using MessagesIdMap = std::map<unsigned, MessagesMap::const_iterator>;

    bool parseSchema(const ProgramOptions& options);

    xmlDocPtr getDoc()
    {
        return m_doc.get();
    }

    TypesMap& getTypes()
    {
        return m_types;
    }

    const TypesMap& getTypes() const
    {
        return m_types;
    }

    MessagesMap& getMessages()
    {
        return m_messages;
    }

    const MessagesIdMap& getMessagesById() const
    {
        return m_messagesById;
    }

    const std::string& getRootPath() const;

    const std::string& getProtocolNamespace() const;

    unsigned getSchemaVersion() const;

    unsigned getSchemaId() const;

    const std::string& getMessageHeaderType() const;

    unsigned getMinRemoteVersion() const;

    const std::string& getEndian() const;

    bool doesElementExist(unsigned introducedSince) const;

    const Type* findType(const std::string& name) const;

    Type* findType(const std::string& name);

    bool isIntroducedType(const std::string& name) const;

    const Type* getBuiltInType(const std::string& name);

    bool isRecordedBuiltInType(const std::string& name) const;

    const Type* getPaddingType(unsigned len);

    const Type* findPaddingType(const std::string& name) const;

    bool isRecordedPaddingType(const std::string& name) const;

    void recordGroupListUsage();
    bool isGroupListRecorded() const;

    bool isPaddingRecorded() const;

    std::list<std::string> getAllUsedBuiltInTypes() const;

    xmlNodePtr createMsgIdEnumNode(const std::string& name, const std::string& encType);
    xmlNodePtr getMsgIdEnumNode() const;

private:
    struct GeneratedTypeInfo
    {
        XmlNodePtr m_node;
        TypePtr m_type;
    };

    using GeneratedTypeMap = std::map<std::string, GeneratedTypeInfo>;

    bool recordTypeRef(xmlNodePtr node);
    bool parseTypes(xmlNodePtr node);
    bool parseMessage(xmlNodePtr node);
    bool processOptions(const ProgramOptions& options);
    bool processOutputDirectory(const ProgramOptions& options);
    bool processNamespace(const ProgramOptions& options);
    bool processForcedSchemaVersion(const ProgramOptions& options);
    bool processMinRemoteVersion(const ProgramOptions& options);
    bool processMessageSchema();

    XmlDocPtr m_doc;
    std::unique_ptr<MessageSchema> m_messageSchema;
    XmlNodePtr m_msgIdEnum;
    TypesMap m_types;
    GeneratedTypeMap m_builtInTypes;
    GeneratedTypeMap m_paddingTypes;
    MessagesMap m_messages;
    MessagesIdMap m_messagesById;
    std::list<std::string> m_groups;
    bool m_groupListUsed = false;
    std::string m_rootDir;
    std::string m_endian;
    std::string m_namespace;
    unsigned m_schemaVersion;
    unsigned m_minRemoteVersion;
};

} // namespace sbe2comms


