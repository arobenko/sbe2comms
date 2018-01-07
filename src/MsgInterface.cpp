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

#include "MsgInterface.h"

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

bool MsgInterface::write()
{
    return writeProtocolDef() && writePluginHeader();
}

bool MsgInterface::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(m_db.getProtocolNamespace(), common::msgInterfaceFileName());
    auto filePath = bf::path(m_db.getRootPath()) / relPath;

    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& msgHeaderType = m_db.getMessageHeaderType();
    assert(m_db.findType(msgHeaderType) != nullptr);

    auto& ns = m_db.getProtocolNamespace();
    out << "/// \\file\n"
           "/// \\brief Contains definition of common \\ref " << common::scopeFor(ns, common::msgInterfaceStr()) << " interface class.\n\n"
           "#pragma once\n\n"
           "#include \"comms/Message.h\"\n"
           "#include \"comms/options.h\"\n" <<
           "#include " << common::localHeader(ns, common::fieldNamespaceNameStr(), msgHeaderType + ".h") << "\n"
           "#include \"" << common::msgIdFileName() << "\"\n\n";


    common::writeProtocolNamespaceBegin(ns, out);

    auto transportScope = common::scopeFor(ns, common::fieldNamespaceStr() + msgHeaderType + common::memembersSuffixStr() + "::");

    out << "/// \\brief Extra transport fields for every message.\n"
           "using ExtraTransportFields =\n" <<
           output::indent(1) << "std::tuple<\n" <<
           output::indent(2) << transportScope << common::blockLengthStr() << "<>,\n" <<
           output::indent(2) << transportScope << common::versionStr() << "<>\n" <<
           output::indent(1) << ">;\n\n";

    auto writeBaseClassDefFunc =
        [this, &out](unsigned ind)
        {
            out << output::indent(ind) << "comms::Message<\n" <<
                   output::indent(ind + 1) << "comms::option::MsgIdType<" << common::msgIdEnumName() << ">,\n" <<
                   output::indent(ind + 1) << m_db.getEndian() << ",\n" <<
                   output::indent(ind + 1) << "comms::option::ExtraTransportFields<ExtraTransportFields>,\n" <<
                   output::indent(ind + 1) << "TOpt...\n" <<
                   output::indent(ind) << ">";

        };

    out << "/// \\brief Common interface class for all the messages.\n"
           "/// \\tparam TOpt Extra options from \\b comms::option namespace.\n"
           "/// \\see \\ref ExtraTransportFields\n"
           "template <typename... TOpt>\n"
           "class " << common::msgInterfaceStr() << " : public\n";
    writeBaseClassDefFunc(1);
    out << "\n"
           "{\n" <<
           output::indent(1) << "using Base =\n";
    writeBaseClassDefFunc(2);
    out << ";\n\n"
           "public:\n" <<
           output::indent(1) << "/// \\brief Allow access to extra transport fields.\n" <<
           output::indent(1) << "/// \\details See definition of \\b COMMS_MSG_TRANSPORT_FIELDS_ACCESS macro\n" <<
           output::indent(1) << "///     related to \\b comms::Message class from COMMS library\n" <<
           output::indent(1) << "///     for details.\n" <<
           output::indent(1) << "COMMS_MSG_TRANSPORT_FIELDS_ACCESS(" << common::blockLengthStr() << ", " << common::versionStr() << ");\n\n" <<
           output::indent(1) << "/// \\brief Set the value of the root block length.\n" <<
           output::indent(1) << "void setBlockLength(std::size_t value)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "auto& blockLengthField = transportField_blockLength();\n" <<
           output::indent(2) << "using BlockLengthFieldType = typename std::decay<decltype(blockLengthField)>::type;\n" <<
           output::indent(2) << "using BlockLengthValueType = typename BlockLengthFieldType::ValueType;\n" <<
           output::indent(2) << "blockLengthField.value() = static_cast<BlockLengthValueType>(value);\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Get the value of the root block length.\n" <<
           output::indent(1) << "std::size_t getBlockLength() const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return transportField_blockLength().value();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Set the value of the schema version.\n" <<
           output::indent(1) << "void setVersion(unsigned value)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "auto& versionField = transportField_version();\n" <<
           output::indent(2) << "using VersionFieldType = typename std::decay<decltype(versionField)>::type;\n" <<
           output::indent(2) << "using VersionValueType = typename VersionFieldType::ValueType;\n" <<
           output::indent(2) << "versionField.value() = static_cast<VersionValueType>(value);\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Get the value of the schema version.\n" <<
           output::indent(1) << "unsigned getVersion() const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return transportField_version().value();\n" <<
           output::indent(1) << "}\n\n" <<
           "};\n\n";

    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

bool MsgInterface::writePluginHeader()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    auto& ns = common::pluginNamespaceNameStr();
    auto relPath = common::pathTo(ns, common::msgInterfaceFileName());
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& protNs = m_db.getProtocolNamespace();
    out << "#pragma once\n\n"
           "#include \"comms_champion/comms_champion.h\"\n"
           "#include " << common::localHeader(protNs, std::string(), common::msgInterfaceFileName()) << "\n\n";

    common::writePluginNamespaceBegin(protNs, out);

    auto protMsgScope = common::scopeFor(protNs, common::msgInterfaceStr());
    out << "template <typename... TOptions>\n"
           "class " <<  common::msgInterfaceStr() << " : public comms_champion::MessageBase<" << protMsgScope << ", TOptions...>\n"
           "{\n"
           "protected:\n" <<
           output::indent(1) << "const QVariantList& extraTransportFieldsPropertiesImpl() const override\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "static const QVariantList Props = createExtraTransportFields();\n" <<
           output::indent(2) << "return Props;\n" <<
           output::indent(1) << "}\n\n"
           "private:\n" <<
           output::indent(1) << "static QVariantMap createFieldProps_" << common::blockLengthStr() << "()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return\n" <<
           output::indent(3) << "comms_champion::property::field::IntValue()\n" <<
           output::indent(4) << ".name(\"" << common::blockLengthStr() << "\")\n" <<
           output::indent(4) << ".readOnly()\n" <<
           output::indent(4) << ".hidden()\n" <<
           output::indent(4) << ".asMap();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "static QVariantMap createFieldProps_" << common::versionStr() << "()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return\n" <<
           output::indent(3) << "comms_champion::property::field::IntValue()\n" <<
           output::indent(4) << ".name(\"" << common::versionStr() << "\")\n" <<
           output::indent(4) << ".serialisedHidden()\n" <<
           output::indent(4) << ".hiddenWhenReadOnly()\n" <<
           output::indent(4) << ".asMap();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "static QVariantList createExtraTransportFields()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "QVariantList props;\n" <<
           output::indent(2) << "props.append(createFieldProps_" << common::blockLengthStr() << "());\n" <<
           output::indent(2) << "props.append(createFieldProps_" << common::versionStr() << "());\n\n" <<
           output::indent(2) << "assert(props.size() == " << common::scopeFor(protNs, common::msgInterfaceStr()) << "<>::TransportFieldIdx_numOfValues);\n" <<
           output::indent(2) << "return props;\n" <<
           output::indent(1) << "}\n"
           "};\n";

    common::writePluginNamespaceEnd(protNs, out);
    return true;
}

} // namespace sbe2comms
