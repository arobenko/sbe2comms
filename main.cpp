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
#include <fstream>

#include <boost/filesystem.hpp>

#include "DB.h"
#include "BuiltIn.h"
#include "common.h"
#include "output.h"
#include "log.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

bool writeBuiltIn(DB& db)
{
    return BuiltIn::write(db);
}

bool writeMessages(DB& db)
{
    auto& messages = db.getMessages();
    for (auto iter = messages.begin(); iter != messages.end(); ++iter) {
        assert(iter->second);
        if (!iter->second->write()) {
            return false;
        }
    }
    return true;
}

bool writeTypes(DB& db)
{
    bf::path root(db.getRootPath());
    bf::path protocolRelDir(db.getProtocolRelDir());
    bf::path protocolDir(root / protocolRelDir);

    boost::system::error_code ec;
    bf::create_directories(protocolDir, ec);
    if (ec) {
        log::error() << "Failed to create \"" << protocolDir.string() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }

    auto fileRelPath = (protocolRelDir / common::fieldsDefFileName()).string();
    log::info() << "Generating " << fileRelPath << std::endl;

    auto filePath = (protocolDir / common::fieldsDefFileName()).string();
    std::ofstream stream(filePath);
    if (!stream) {
        log::error() << "Failed to create " << filePath << std::endl;
        return false;
    }

    stream << "/// \\file\n"
              "/// \\brief Contains definition of all the field types\n"
              "\n\n"
              "#include <cstdint>\n";
    std::set<std::string> extraIncludes;
    for (auto& t : db.getTypes()) {
        assert(t.second);
        t.second->updateExtraIncludes(extraIncludes);
    }

    for (auto& i : extraIncludes) {
        stream << "#include " << i << '\n';
    }
    auto& ns = db.getProtocolNamespace();
    stream << "\n"
              "#include \"comms/fields.h\"\n"
              "#include \"comms/Field.h\"\n"
              "#include \"" << common::pathTo(ns, common::msgIdFileName()) << "\"\n\n";
    if (!ns.empty()) {
        stream << "namespace " << ns << "\n"
                  "{\n\n";
    }

    stream << "namespace field\n"
              "{\n\n"
              "/// \\brief Definition of common base class of all the fields.\n"
              "using FieldBase = comms::Field<" << db.getEndian() << ">;\n\n";


    bool result = true;
    for (auto* t : db.getTypesList()) {
        result = t->write(stream) && result;
    }

    stream << "} // namespace field\n\n";

    if (!ns.empty()) {
        stream << "} // namespace " << ns << "\n\n";
    }

    if (!stream.good()) {
        log::error() << "The file " << fileRelPath << "hasn't been written properly!" << std::endl;
        return false;
    }

    return result;
}

bool writeDefaultOptions(DB& db)
{
    bf::path root(db.getRootPath());
    bf::path protocolRelDir(db.getProtocolRelDir());
    bf::path protocolDir(root / protocolRelDir);

    boost::system::error_code ec;
    bf::create_directories(protocolDir, ec);
    if (ec) {
        log::error() << "Failed to create \"" << protocolDir.string() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }

    auto fileRelPath = (protocolRelDir / common::defaultOptionsFileName()).string();
    log::info() << "Generating " << fileRelPath << std::endl;

    auto filePath = (protocolDir / common::defaultOptionsFileName()).string();
    std::ofstream stream(filePath);
    if (!stream) {
        log::error() << "Failed to create " << filePath << std::endl;
        return false;
    }

    stream << "/// \\file\n"
              "/// \\brief Contains definition of default options.\n"
              "\n\n"
              "#include \"comms/options.h\"\n\n";

    auto& ns = db.getProtocolNamespace();
    if (!ns.empty()) {
        stream << "namespace " << ns << "\n"
                  "{\n\n";
    }

    stream << "struct " << common::defaultOptionsStr() << "\n"
              "{\n" <<
              output::indent(1) << "struct " << common::fieldNamespaceNameStr() << '\n' <<
              output::indent(1) << "{\n";

    bool result = true;
    auto fieldsScope = ns + "::" + common::fieldNamespaceStr();
    for (auto& t : db.getTypes()) {
        result = t.second->writeDefaultOptions(stream, 2, fieldsScope) && result;
    }

    stream << output::indent(1) << "}; // " << common::fieldNamespaceNameStr() << "\n\n" <<
              output::indent(1) << "struct " << common::messageNamespaceNameStr() << '\n' <<
              output::indent(1) << "{\n";

    auto messagesScope = ns + "::" + common::messageNamespaceStr();
    for (auto& m : db.getMessages()) {
        assert(m.second);
        result = m.second->writeDefaultOptions(stream, 2, messagesScope) && result;
    }

    stream << output::indent(1) << "}; // " << common::messageNamespaceNameStr() << "\n\n" <<
              "}; // DefaultOptions\n\n";

    if (!ns.empty()) {
        stream << "} // namespace " << ns << "\n\n";
    }

    if (!stream.good()) {
        log::error() << "The file " << fileRelPath << "hasn't been written properly!" << std::endl;
        return false;
    }

    return result;
}

} // namespace sbe2comms

int main(int argc, const char* argv[])
{

    if (argc < 2) {
        sbe2comms::log::error() << "Wrong number of arguments" << std::endl;
        return -1;
    }

    sbe2comms::DB db;
    bool result =
        db.parseSchema(argv[1]) &&
        sbe2comms::writeBuiltIn(db) &&
        sbe2comms::writeMessages(db) &&
        sbe2comms::writeTypes(db) &&
        sbe2comms::writeDefaultOptions(db);

    if (result) {
        std::cout << "SUCCESS" << std::endl;
        return 0;
    }

    return -1;
}
