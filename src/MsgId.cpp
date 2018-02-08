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

#include "MsgId.h"

#include <fstream>
#include <boost/filesystem.hpp>

#include "DB.h"
#include "common.h"
#include "log.h"
#include "prop.h"
#include "output.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

bool MsgId::write()
{
    return writeProtocolDef();
}

bool MsgId::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(m_db.getProtocolNamespace(), common::msgIdFileName());
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& ns = m_db.getProtocolNamespace();
    out << "/// \\file\n"
           "/// \\brief Contains definition of \\ref " << common::scopeFor(ns, common::msgIdEnumName()) << " enumeration.\n\n"
           "#pragma once\n\n" <<
           "#include <cstdint>\n\n";

    common::writeProtocolNamespaceBegin(ns, out);

    auto* msgIdNode = m_db.getMsgIdEnumNode();
    auto props = xmlParseNodeProps(msgIdNode, m_db.getDoc());
    auto& encType = prop::encodingType(props);
    auto& underlyingType = common::primitiveTypeToStdInt(encType);
    assert(!underlyingType.empty());
    out << "/// \\brief Enumeration of message ID value.\n"
           "enum " << common::msgIdEnumName() << " : " << underlyingType << '\n' <<
           "{\n";
    auto prefix = common::msgIdEnumName() + '_';
    auto& msgs = m_db.getMessagesById();
    for (auto& m : msgs) {
        auto& msgName = m.second->first;
        auto id = static_cast<std::intmax_t>(m.first);
        out << output::indent(1) << prefix << msgName << " = " << common::num(id) << ", ///< ID of message \\ref " <<
               common::messageNamespaceStr() << msgName << '\n';
    }
    out << "}; // " << common::msgIdEnumName() << "\n\n";
    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
