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
#include "ProgramOptions.h"
#include "BuiltIn.h"
#include "MsgId.h"
#include "MsgInterface.h"
#include "AllMessages.h"
#include "AllFields.h"
#include "MessageHeaderLayer.h"
#include "OpenFramingHeaderLayer.h"
#include "TransportFrame.h"
#include "FieldBase.h"
#include "common.h"
#include "output.h"
#include "log.h"
#include "Cmake.h"
#include "TransportMessage.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

bool writeBuiltIn(DB& db)
{
    BuiltIn obj(db);
    return obj.write();
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
    FieldBase fieldBase(db);
    bool result = fieldBase.write();
    for (auto& t : db.getTypes()) {
        assert(t.second);
        result = t.second->writeProtocolDef() && result;
    }

    AllFields allFields(db);
    result = allFields.write() && result;
    return result;
}

bool writeDefaultOptions(DB& db)
{
    if (!common::createProtocolDefDir(db.getRootPath(), db.getProtocolNamespace())) {
        return false;
    }

    auto fileRelPath = common::protocolDirRelPath(db.getProtocolNamespace(), common::defaultOptionsFileName());
    log::info() << "Generating " << fileRelPath << std::endl;

    auto filePath = (bf::path(db.getRootPath()) / fileRelPath).string();

    std::ofstream stream(filePath);
    if (!stream) {
        log::error() << "Failed to create " << filePath << std::endl;
        return false;
    }

    stream << "/// \\file\n"
              "/// \\brief Contains definition of default options.\n"
              "\n\n"
              "#pragma once\n\n"
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

bool writeMsgId(DB& db)
{
    MsgId msgId(db);
    return msgId.write();
}

bool writeMsgInterface(DB& db)
{
    MsgInterface msgInterface(db);
    return msgInterface.write();
}

bool writeAllMessages(DB& db)
{
    AllMessages obj(db);
    return obj.write();
}

bool writeMessageHeaderLayer(DB& db)
{
    MessageHeaderLayer obj(db);
    return obj.write();
}

bool writeOpenFramingHeaderLayer(DB& db)
{
    OpenFramingHeaderLayer obj(db);
    return obj.write();
}

bool writeTransportFrame(DB& db)
{
    TransportFrame obj(db);
    return obj.write();
}

bool writeTransportMessage(DB& db)
{
    TransportMessage obj(db);
    return obj.write();
}

bool writeCmake(DB& db)
{
    Cmake obj(db);
    return obj.write();
}

} // namespace sbe2comms

int main(int argc, const char* argv[])
{
    sbe2comms::ProgramOptions options;
    options.parse(argc, argv);
    if (options.helpRequested()) {
        std::cout << "Usage:\n\t" << argv[0] << " [OPTIONS] schema_file\n";
        options.printHelp(std::cout);
        return 0;
    }

    sbe2comms::DB db;
    bool result =
        db.parseSchema(options) &&
        sbe2comms::writeBuiltIn(db) &&
        sbe2comms::writeMessages(db) &&
        sbe2comms::writeTypes(db) &&
        sbe2comms::writeDefaultOptions(db) &&
        sbe2comms::writeMsgId(db) &&
        sbe2comms::writeMsgInterface(db) &&
        sbe2comms::writeAllMessages(db) &&
        sbe2comms::writeMessageHeaderLayer(db) &&
        sbe2comms::writeOpenFramingHeaderLayer(db) &&
        sbe2comms::writeTransportFrame(db) &&
        sbe2comms::writeTransportMessage(db) &&
        sbe2comms::writeCmake(db)
    ;

    if (result) {
        std::cout << "SUCCESS" << std::endl;
        return 0;
    }

    return -1;
}
