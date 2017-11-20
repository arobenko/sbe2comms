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

#include "AllFields.h"

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

void openNamespaces(std::ostream& out, DB& db)
{
    common::writePluginNamespaceBegin(db.getProtocolNamespace(), out);
    out << "namespace " << common::fieldNamespaceNameStr() << "\n"
           "{\n\n";
}

void closeNamespaces(std::ostream& out, DB& db)
{
    out << "} // namespace " << common::fieldNamespaceNameStr() << "\n\n";

    common::writePluginNamespaceEnd(db.getProtocolNamespace(), out);
}

} // namespace

bool AllFields::write()
{
    return writePluginHeader() && writePluginDef();
}

bool AllFields::writePluginHeader()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    auto relPath = common::pluginNamespaceNameStr() + '/' + common::fieldHeaderFileName();
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }


    out << "#pragma once\n\n"
           "#include <QtCore/QVariantMap>\n\n";

    out << '\n';
    openNamespaces(out, m_db);

    for (auto& t : m_db.getTypes()) {
        out << "QVariantMap createProps_" << t.first << "(const char* " << common::fieldNameParamNameStr() << ");\n";
    }

    out << '\n';
    closeNamespaces(out, m_db);
    return true;
}

bool AllFields::writePluginDef()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    auto relPath = common::pluginNamespaceNameStr() + '/' + common::fieldDefFileName();
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "#include \"" << common::fieldHeaderFileName() << "\"\n\n"
           "#include <QtCore/QString>\n"
           "#include <QtCore/QVariantList>\n\n"
           "#include \"comms_champion/comms_champion.h\"\n\n";

    for (auto& t : m_db.getTypes()) {
        auto fileDefPath = common::fieldNamespaceNameStr() + '/' + t.first + ".h";
        auto fullDefPath = common::pathTo(m_db.getProtocolNamespace(), fileDefPath);
        out << "#include \"" << fullDefPath << "\"\n";
    }

    out << '\n';

    openNamespaces(out, m_db);

    for (auto& t : m_db.getTypes()) {
        out << "QVariantMap createProps_" << t.first << "(const char* " << common::fieldNameParamNameStr() << ")\n"
               "{\n";
        t.second -> writePluginProperties(out, 1);
        out << "}\n\n";
    }

    closeNamespaces(out, m_db);
    return true;
}

} // namespace sbe2comms
