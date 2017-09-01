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

#include <iostream>

#include <boost/filesystem.hpp>

#include "DB.h"
#include "get.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

bool writeMessages(DB& db)
{
    bf::path root(get::rootPath(db));
    bf::path protocolRelDir(get::protocolRelDir(db));
    bf::path messagesDir(root / protocolRelDir / get::messageDirName());

    boost::system::error_code ec;
    bf::create_directories(messagesDir, ec);
    if (ec) {
        std::cerr << "ERROR: Failed to create \"" << messagesDir.string() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }

    const std::string Ext(".h");
    for (auto iter = db.m_messages.begin(); iter != db.m_messages.end(); ++iter) {
        auto filename = iter->first + Ext;
        auto relPath = protocolRelDir / get::messageDirName() / filename;
        std::cout << "INFO: Generating " << relPath.string() << std::endl;

        auto filePath = messagesDir / filename;
        if (!iter->second.write(filePath.string(), db)) {
            return false;
        }
    }
    return true;
}

} // namespace sbe2comms

int main(int argc, const char* argv[])
{

    if (argc < 2) {
        std::cerr << "Wrong number of arguments" << std::endl;
        return -1;
    }

    sbe2comms::DB db;
    bool result =
        sbe2comms::parseSchema(argv[1], db) &&
        sbe2comms::writeMessages(db);

    if (result) {
        std::cout << "SUCCESS" << std::endl;
        return 0;
    }

    return -1;
}
