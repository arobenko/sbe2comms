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

#include "get.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace get
{

const std::string& rootPath(DB& db)
{
    if (db.m_cache.m_rootDir.empty()) {
        // TODO: program options
        db.m_cache.m_rootDir = bf::current_path().string();
    }

    assert(!db.m_cache.m_rootDir.empty());
    return db.m_cache.m_rootDir;
}

const std::string& messageDirName()
{
    static const std::string Name("message");
    return Name;
}

const std::string& includeDirName()
{
    static const std::string Name("include");
    return Name;
}

const std::string& fieldsDefFileName()
{
    static const std::string Name("field.h");
    return Name;
}

const std::string& nameProperty()
{
    static const std::string Str("name");
    return Str;
}

const std::string& emptyString()
{
    static const std::string Str;
    return Str;
}

const std::string& unknownValueString()
{
    static const std::string Str("???;");
    return Str;
}

const std::string& protocolNamespace(DB& db)
{
    if (db.m_cache.m_namespace) {
        return *db.m_cache.m_namespace;
    }

    assert(db.m_messageSchema);
    auto package = db.m_messageSchema->package();
    ba::replace_all(package, " ", "_");
    db.m_cache.m_namespace = std::move(package);
    return *db.m_cache.m_namespace;
}

const std::string& protocolRelDir(DB& db)
{
    if (db.m_cache.m_protocolRelDir.empty()) {
        bf::path path(includeDirName());
        auto& ns = protocolNamespace(db);
        if (!ns.empty()) {
            path /= ns;
        }

        db.m_cache.m_protocolRelDir = path.string();
    }

    assert(!db.m_cache.m_protocolRelDir.empty());
    return db.m_cache.m_protocolRelDir;
}

unsigned schemaVersion(DB& db)
{
    auto& val = db.m_cache.m_schemaVersion;
    if (!val) {
        // TODO: check options for override
        assert(db.m_messageSchema);
        val = db.m_messageSchema->version();
    }

    return *val;
}

const std::string& endian(DB& db)
{
    auto& val = db.m_cache.m_endian;
    if (!val.empty()) {
        return val;
    }

    assert(db.m_messageSchema);
    auto& byteOrder = db.m_messageSchema->byteOrder();

    static const std::string BigEndianStr("bigEndian");
    if (byteOrder == BigEndianStr) {
        val = "comms::option::BigEndian";
        return val;
    }

    val = "comms::option::LittleEndian";
    return val;
}

} // namespace get

} // namespace sbe2comms
