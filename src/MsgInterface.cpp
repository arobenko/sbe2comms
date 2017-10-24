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

#include "MsgInterface.h"

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

bool MsgInterface::write()
{
    return writeProtocolDef();
}

bool MsgInterface::writeProtocolDef()
{
    bf::path root(m_db.getRootPath());
    bf::path protocolRelDir(m_db.getProtocolRelDir());
    bf::path protocolDir = root / protocolRelDir;

    boost::system::error_code ec;
    bf::create_directories(protocolDir, ec);
    if (ec) {
        log::error() << "Failed to create \"" << protocolRelDir.string() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }

    auto relPath = protocolRelDir / common::msgInterfaceFileName();
    log::info() << "Generating " << relPath.string() << std::endl;

    auto filePath = protocolDir / common::msgInterfaceFileName();;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << relPath.string() << std::endl;
        return false;
    }

    out << "/// \\file\n"
           "/// \\brief Contains definition of common \\ref Message interface class.\n\n"
           "#pragma once\n\n"
           "#include \"comms/Message.h\"\n"
           "#include \"comms/options.h\"\n"
           "#include \"" << common::msgIdFileName() << "\"\n\n";


    auto& ns = m_db.getProtocolNamespace();
    common::writeProtocolNamespaceBegin(ns, out);

    out << "/// \\brief Common interface class for all the messages.\n"
           "/// \\tparam TOpt Extra options from \\b comms::option namespace.\n"
           "template <typename... TOpt>\n"
           "class Message : public\n" <<
           output::indent(1) << "comms::Message<\n" <<
           output::indent(2) << "comms::option::MsgIdType<" << common::msgIdEnumName() << ">,\n" <<
           output::indent(2) << m_db.getEndian() << ",\n" <<
           output::indent(2) << "Opt...\n" <<
           output::indent(1) << ">\n" <<
           "{\n"
           "public:\n" <<
           output::indent(1) << "/// \\brief Set the value of the root block length.\n" <<
           output::indent(1) << "void setBlockLength(std::size_t value) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "m_blockLength = value;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Get the value of the root block length.\n" <<
           output::indent(1) << "std::size_t getBlockLength() const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return m_blockLength;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Set the value of the schema version.\n" <<
           output::indent(1) << "void setVersion(unsigned value) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "m_version = value;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Get the value of the schema version.\n" <<
           output::indent(1) << "unsigned getVersion() const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return m_version;\n" <<
           output::indent(1) << "}\n\n" <<
           "private:\n" <<
           output::indent(1) << "std::size_t m_blockLength = 0U;\n" <<
           output::indent(1) << "unsigned m_version = " << m_db.getSchemaVersion() << "U;\n" <<
           "};\n\n";

    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
