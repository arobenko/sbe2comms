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

#include "Plugin.h"

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "common.h"
#include "log.h"
#include "output.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

bool Plugin::write()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    return
        writeMetaFile(false) &&
        writeMetaFile(true) &&
        writeHeader(false) &&
        writeHeader(true) &&
        writeSrc(false) &&
        writeSrc(true);
}

bool Plugin::writeMetaFile(bool openFrame)
{
    auto* name = &common::messageHeaderFrameStr();
    std::string shortDesc;
    std::string desc("with message header only");

    if (openFrame) {
        name = &common::openFramingHeaderFrameStr();
        shortDesc = " (Open Frame)";
        desc = "with both message and simple open framing headers";
    }
    auto relPath = common::pluginNamespaceNameStr() + '/' + "plugin_" + *name + ".json";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& protName = m_db.getPackageName();
    auto nameStr = protName + " Protocol";
    out << "{\n" <<
           output::indent(1) << "\"name\" : \"" << nameStr << shortDesc << "\",\n" <<
           output::indent(1) << "\"desc\" : [\n" <<
           output::indent(2) << '\"' << nameStr << ' ' << desc << ".\"\n" <<
           output::indent(1) << "],\n" <<
           output::indent(1) << "\"type\" : \"protocol\"\n"
           "}\n";

    return true;
}

bool Plugin::writeHeader(bool openFrame)
{
    auto protName = m_db.getPackageName();
    ba::replace_all(protName, " ", "_");

    auto* name = &common::messageHeaderFrameStr();
    std::string idSuffix;
    if (openFrame) {
        name = &common::openFramingHeaderFrameStr();
        idSuffix = ".OpenFrame";
    }

    auto className = *name + common::pluginNameStr();

    auto relPath = common::pluginNamespaceNameStr() + '/' + className + ".h";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "#pragma once\n\n"
           "#include <QtCore/QObject>\n"
           "#include <QtCore/QtPlugin>\n\n"
           "#include \"comms_champion/comms_champion.h\"\n\n";

    auto& ns = m_db.getProtocolNamespace();
    common::writePluginNamespaceBegin(ns, out);
    out << "class " << className << " : public comms_champion::Plugin\n"
           "{\n" <<
           output::indent(1) << "Q_OBJECT\n" <<
           output::indent(1) << "Q_PLUGIN_METADATA(IID \"" << protName << idSuffix << "\" FILE \"plugin_" << *name << ".json\")\n" <<
           output::indent(1) << "Q_INTERFACES(comms_champion::Plugin)\n\n" <<
           "public:\n" <<
           output::indent(1) << className << "();\n" <<
           output::indent(1) << '~' << className << "();\n" <<
           "};\n\n";

    common::writePluginNamespaceEnd(ns, out);
    return true;
}

bool Plugin::writeSrc(bool openFrame)
{
    auto* name = &common::messageHeaderFrameStr();
    if (openFrame) {
        name = &common::openFramingHeaderFrameStr();
    }

    auto className = *name + common::pluginNameStr();

    auto relPath = common::pluginNamespaceNameStr() + '/' + className + ".cpp";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto header = *name + common::pluginNameStr() + ".h";
    out << "#include " << common::localHeader(common::pluginNamespaceNameStr(), header) << '\n' <<
           "#include " << common::localHeader(common::pluginNamespaceNameStr(), common::protocolNameStr() + ".h") << "\n\n" <<
           "namespace cc = comms_champion;\n\n";

    auto& ns = m_db.getProtocolNamespace();
    common::writePluginNamespaceBegin(ns, out);

    out << className << "::" << className << "()\n"
           "{\n" <<
           output::indent(1) << "pluginProperties()\n" <<
           output::indent(2) << ".setProtocolCreateFunc(\n" <<
           output::indent(3) << "[this]() -> cc::ProtocolPtr\n" <<
           output::indent(3) << "{\n" <<
           output::indent(4) << "return cc::ProtocolPtr(new Protocol());\n" <<
           output::indent(3) << "});\n" <<
           "}\n\n" <<
           className << "::~" << className << "() = default;\n\n";
    common::writePluginNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
