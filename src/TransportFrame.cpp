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

#include "TransportFrame.h"

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

bool TransportFrame::write()
{
    return writeProtocolDef() && writePluginDef();
}

bool TransportFrame::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(m_db.getProtocolNamespace(), common::transportFrameFileName());
    auto filePath = bf::path(m_db.getRootPath()) / relPath;

    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& messageHeaderType = m_db.getMessageHeaderType();
    if (messageHeaderType.empty()) {
        log::error() << "Unknown message header type." << std::endl;
        return false;
    }

    out << "/// \\file\n"
           "/// \\brief Contains definition of transport frames.\n\n"
           "#pragma once\n\n"
           "#include <cstdint>\n\n"
           "#include \"comms/protocol/MsgDataLayer.h\"\n"
           "#include \"comms/options.h\"\n"
           "#include \"comms/field/ArrayList.h\"\n"
           "#include \"" << common::messageHeaderLayerFileName() << "\"\n"
           "#include \"" << common::openFramingHeaderLayerFileName() << "\"\n"
           "#include \"" << common::defaultOptionsFileName() << "\"\n\n";


    auto& ns = m_db.getProtocolNamespace();
    common::writeProtocolNamespaceBegin(ns, out);

    out << "/// \\brief Definition of transport frame involving only message header\n"
           "///     (\\ref " << common::fieldNamespaceStr() << messageHeaderType << ").\n"
           "/// \\tparam TMsgBase Common base (interface) class of all the \\b input messages.\n"
           "/// \\tparam TMessages All the message types that need to be recognized in the\n"
           "///     input and created.\n"
           "/// \\tparam TOpt Protocol definition options, expected to be \\ref DefaultOptions or\n"
           "///     derived class with similar types inside.\n"
           "/// \\tparam TFactoryOpt Options from \\b comms::option namespace \n"
           "///     to be passed to \\b comms::MsgFactory object\n"
           "///     contained by \\ref " << common::messageHeaderLayerStr() << ". It controls the way the message\n"
           "///     objects are created.\n"
           "/// \\tparam TDataStorageOpt Extra options from \\b comms::option namespace\n" <<
           "///     to be passed to raw data storage field used by \\b comms::protocol::MsgDataLayer.\n"
           "///     \\b NOTE, that this field is used only when \"cached\" read write operations\n"
           "///     are performed, where the read/written raw data needs to be stored for\n"
           "///     future reference or display. It is not used in normal read/write operations.\n"
           "template <\n" <<
           output::indent(1) << "typename TMsgBase,\n" <<
           output::indent(1) << "typename TMessages,\n" <<
           output::indent(1) << "typename TOpt = DefaultOptions,\n" <<
           output::indent(1) << "typename TFactoryOpt = comms::option::EmptyOption,\n" <<
           output::indent(1) << "typename TDataStorageOpt = comms::option::EmptyOption\n" <<
           ">\n"
           "using " << common::messageHeaderFrameStr() << " =\n" <<
           output::indent(1) << common::messageHeaderLayerStr() << "<\n" <<
           output::indent(2) << "TMsgBase,\n" <<
           output::indent(2) << "TMessages,\n" <<
           output::indent(2) << "comms::protocol::MsgDataLayer<\n" <<
           output::indent(3) << "comms::field::ArrayList<\n" <<
           output::indent(4) << common::fieldBaseFullScope(ns) << ",\n" <<
           output::indent(4) << "std::uint8_t,\n" <<
           output::indent(4) << "TDataStorageOpt\n" <<
           output::indent(3) << ">\n" <<
           output::indent(2) << ">,\n" <<
           output::indent(2) << common::messageHeaderLayerStr() << common::optFieldSuffixStr() << "<TOpt>,\n" <<
           output::indent(2) << "TFactoryOpt\n" <<
           output::indent(1) << ">;\n\n" <<
           "/// \\brief Definition of transport frame involving both message header\n"
           "///     and simple open framing header.\n"
           "/// \\tparam TMsgBase Common base (interface) class of all the \\b input messages.\n"
           "/// \\tparam TMessages All the message types that need to be recognized in the\n"
           "///     input and created.\n"
           "/// \\tparam TOpt Protocol definition options, expected to be \\ref DefaultOptions or\n"
           "///     derived class with similar types inside.\n"
           "/// \\tparam TFactoryOpt Options from \\b comms::option namespace \n"
           "///     to be passed to \\b comms::MsgFactory object\n"
           "///     contained by \\ref " << common::messageHeaderLayerStr() << ". It controls the way the message\n"
           "///     objects are created.\n"
           "/// \\tparam TDataStorageOpt Extra options from \\b comms::option namespace\n" <<
           "///     to be passed to raw data storage field used by \\b comms::protocol::MsgDataLayer.\n"
           "///     \\b NOTE, that this field is used only when \"cached\" read write operations\n"
           "///     are performed, where the read/written raw data needs to be stored for\n"
           "///     future reference or display. It is not used in normal read/write operations.\n"
           "template <\n" <<
           output::indent(1) << "typename TMsgBase,\n" <<
           output::indent(1) << "typename TMessages,\n" <<
           output::indent(1) << "typename TOpt = DefaultOptions,\n" <<
           output::indent(1) << "typename TFactoryOpt = comms::option::EmptyOption,\n" <<
           output::indent(1) << "typename TDataStorageOpt = comms::option::EmptyOption\n" <<
           ">\n"
           "using " << common::openFramingHeaderFrameStr() << " =\n" <<
           output::indent(1) << common::openFramingHeaderLayerStr() << "<\n" <<
           output::indent(2) << common::messageHeaderFrameStr() << "<TMsgBase, TMessages, TOpt, TFactoryOpt, TDataStorageOpt>,\n" <<
           output::indent(2) << common::openFramingHeaderLayerStr() << common::optFieldSuffixStr() << "<TOpt>\n" <<
           output::indent(1) << ">;\n\n";

    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

bool TransportFrame::writePluginDef()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    auto relPath = common::pluginNamespaceNameStr() + '/' + common::transportFrameFileName();
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& ns = m_db.getProtocolNamespace();
    auto& pluginNs = common::pluginNamespaceNameStr();

    out << "#pragma once\n\n"
           "#include " << common::localHeader(ns, common::transportFrameFileName()) << '\n' <<
           "#include " << common::localHeader(pluginNs, common::msgInterfaceFileName()) << '\n' <<
           "#include " << common::localHeader(pluginNs, common::allMessagesFileName()) << "\n\n";

    common::writePluginNamespaceBegin(ns, out);

    out << "using " << common::messageHeaderFrameStr() << " = \n" <<
           output::indent(1) << common::scopeFor(ns, common::messageHeaderFrameStr()) << "<\n" <<
           output::indent(2) << common::scopeFor(pluginNs, common::msgInterfaceStr()) << "<>,\n" <<
           output::indent(2) << common::scopeFor(pluginNs, common::allMessagesStr()) << '\n' <<
           output::indent(1) << ">;\n\n";

    out << "using " << common::openFramingHeaderFrameStr() << " = \n" <<
           output::indent(1) << common::scopeFor(ns, common::openFramingHeaderFrameStr()) << "<\n" <<
           output::indent(2) << common::scopeFor(pluginNs, common::msgInterfaceStr()) << "<>,\n" <<
           output::indent(2) << common::scopeFor(pluginNs, common::allMessagesStr()) << '\n' <<
           output::indent(1) << ">;\n\n";

    common::writePluginNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
