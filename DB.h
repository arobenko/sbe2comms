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

struct DB
{
    XmlDocPtr m_doc;
    std::unique_ptr<MessageSchema> m_messageSchema;
    std::map<std::string, TypePtr> m_types;
    std::map<std::string, Message> m_messages;

    struct Cache {
        std::string m_rootDir;
        std::string m_protocolRelDir;
        boost::optional<std::string> m_namespace;
    } m_cache;
};

bool parseSchema(std::string filename, sbe2comms::DB& db);
//bool updateConfig(DB& db);

} // namespace sbe2comms


