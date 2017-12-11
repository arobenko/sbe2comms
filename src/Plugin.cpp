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

#include "DB.h"
#include "common.h"
#include "log.h"
#include "output.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

bool Plugin::write()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    return
        writeMetaFile(false) &&
        writeMetaFile(true);
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

} // namespace sbe2comms
