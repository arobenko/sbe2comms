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

namespace sbe2comms
{

class DB
{
public:
    using TypesMap = std::map<std::string, TypePtr>;
    using MessagesMap = std::map<std::string, Message>;

    bool parseSchema(std::string filename);

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

    const std::string& getRootPath();

    const std::string& getProtocolNamespace();

    const std::string& getProtocolRelDir();

    unsigned getSchemaVersion();

    unsigned getMinRemoteVersion();

    const std::string& getEndian();

    bool doesElementExist(unsigned introducedSince, unsigned deprecatedSince);

    const Type* findType(const std::string& name) const;

    Type* findType(const std::string& name);

    const Type* getBuiltInType(const std::string& name);

    bool isRecordedBuiltInType(const std::string& name) const;

    const Type* getPaddingType(unsigned len);

    const Type* findPaddingType(const std::string& name) const;

    bool isRecordedPaddingType(const std::string& name) const;

    void recordGroupListUsage();

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

    XmlDocPtr m_doc;
    std::unique_ptr<MessageSchema> m_messageSchema;
    TypesMap m_types;
    GeneratedTypeMap m_builtInTypes;
    GeneratedTypeMap m_paddingTypes;
    MessagesMap m_messages;
    std::list<std::string> m_groups;
    bool m_groupListUsed = false;

    struct Cache {
        std::string m_rootDir;
        std::string m_protocolRelDir;
        std::string m_endian;
        boost::optional<std::string> m_namespace;
        boost::optional<unsigned> m_schemaVersion;
    } m_cache;
};

} // namespace sbe2comms


