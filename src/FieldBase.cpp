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

#include "FieldBase.h"

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

namespace
{

void writeFileHeader(DB& db, std::ostream& out)
{
    auto& ns = db.getProtocolNamespace();
    out << "/// \\file\n"
           "/// \\brief Contains definition of \\ref " << common::scopeFor(ns, common::fieldNamespaceStr() + common::fieldBaseStr()) << " type.\n\n"
           "#pragma once\n\n"
           "#include \"comms/Field.h\"\n"
           "#include \"comms/options.h\"\n\n";
}

void openNamespaces(std::ostream& out, DB& db)
{
    common::writeProtocolNamespaceBegin(db.getProtocolNamespace(), out);

    out << "namespace " << common::fieldNamespaceNameStr() << "\n"
            "{\n"
            "\n";
}

void closeNamespaces(std::ostream& out, DB& db)
{
    out << "} // namespace " << common::fieldNamespaceNameStr() << "\n"
            "\n";

    common::writeProtocolNamespaceEnd(db.getProtocolNamespace(), out);
}

} // namespace

bool FieldBase::write()
{
    return writeProtocolDef();
}

bool FieldBase::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace(), common::fieldDirName())) {
        return false;
    }

    auto fieldDirRelPath =
            common::protocolDirRelPath(m_db.getProtocolNamespace(), common::fieldDirName());

    auto filename = common::fieldBaseFileName();
    auto relPath = bf::path(fieldDirRelPath) / filename;
    auto filePath = bf::path(m_db.getRootPath()) / relPath;

    log::info() << "Generating " << relPath.string() << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    writeFileHeader(m_db, out);
    openNamespaces(out, m_db);
    out << "/// \\brief Definition of common base class of all the fields.\n"
           "using " << common::fieldBaseStr() << " = comms::Field<" << m_db.getEndian() << ">;\n\n";
    closeNamespaces(out, m_db);
    return true;
}

} // namespace sbe2comms
