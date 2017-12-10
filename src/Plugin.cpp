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
        writeMetaFile(common::messageHeaderFrameStr()) &&
        writeMetaFile(common::openFramingHeaderFrameStr());
}

bool Plugin::writeMetaFile(const std::string& name)
{
    auto relPath = common::pluginNamespaceNameStr() + '/' + "plugin_" + name + ".json";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    return true;
}

} // namespace sbe2comms
