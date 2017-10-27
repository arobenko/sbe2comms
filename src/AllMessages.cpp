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

#include "AllMessages.h"

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

bool AllMessages::write()
{
    return writeProtocolDef();
}

bool AllMessages::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(m_db.getProtocolNamespace(), common::allMessagesFileName());
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }


    out << "/// \\file\n"
           "/// \\brief Contains definition of all the message classes bundled in std::tuple.\n\n"
           "#pragma once\n\n"
           "#include <tuple>\n\n"
           "#include \"" << common::defaultOptionsFileName() << "\"\n";
    std::string prefix = m_db.getProtocolNamespace();
    if (!prefix.empty()) {
        prefix += '/';
    }
    prefix += common::messageNamespaceNameStr();
    prefix += '/';
    for (auto& m : m_db.getMessagesById()) {
        out << "#include \"" << prefix << m.second->first << ".h\"\n";
    }
    out << "\n\n";

    auto& ns = m_db.getProtocolNamespace();
    common::writeProtocolNamespaceBegin(ns, out);

    out << "/// \\brief All the protocol messages bundled in std::tuple.\n"
           "/// \\tparam TMsgBase Common base (interface) class of all the messages.\n"
           "/// \\tparam TOpt Extra options, expected to be of the same format as \\ref " << common::defaultOptionsStr() << ".\n" <<
           "template <typename TMsgBase, typename TOpt = " << common::defaultOptionsStr() << ">\n"
           "using " << common::allMessagesStr() << " = std::tuple<\n";
    std::size_t remCount = m_db.getMessagesById().size();
    for (auto& m : m_db.getMessagesById()) {
        out << output::indent(1) << common::messageNamespaceNameStr() << m.second->first << "<TMsgBase, TOpt>";
        --remCount;
        if (0 < remCount) {
            out << ',';
        }
        out << '\n';
    }
    out << ">;\n\n";
    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
