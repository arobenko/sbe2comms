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

#include "Protocol.h"

#include <fstream>
#include <boost/filesystem.hpp>

#include "DB.h"
#include "common.h"
#include "log.h"
#include "output.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

bool Protocol::write()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    return
        writeHeader() &&
        writeSrc(common::messageHeaderFrameStr()) &&
        writeSrc(common::openFramingHeaderFrameStr());
}

bool Protocol::writeHeader()
{
    auto relPath = common::pluginNamespaceNameStr() + '/' + common::protocolNameStr() + ".h";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "#pragma once\n\n"
           "#include <memory>\n\n" <<
           "#include \"comms_champion/comms_champion.h\"\n\n";

    auto& ns = m_db.getProtocolNamespace();
    common::writePluginNamespaceBegin(ns, out);
    out << "class " << common::protocolNameStr() << "Impl;\n" <<
           "class " << common::protocolNameStr() << " : public comms_champion::Protocol\n"
           "{\n"
           "public:\n" <<
           output::indent(1) << common::protocolNameStr() << "();\n" <<
           output::indent(1) << "virtual ~" << common::protocolNameStr() << "();\n\n"
           "protected:\n" <<
           output::indent(1) << "virtual const QString& nameImpl() const override;\n" <<
           output::indent(1) << "virtual MessagesList readImpl(const comms_champion::DataInfo& dataInfo, bool final) override;\n" <<
           output::indent(1) << "virtual comms_champion::DataInfoPtr writeImpl(comms_champion::Message& msg) override;\n" <<
           output::indent(1) << "virtual MessagesList createAllMessagesImpl() override;\n" <<
           output::indent(1) << "virtual comms_champion::MessagePtr createMessageImpl(const QString& idAsString, unsigned idx) override;\n" <<
           output::indent(1) << "virtual UpdateStatus updateMessageImpl(comms_champion::Message& msg) override;\n" <<
           output::indent(1) << "virtual comms_champion::MessagePtr cloneMessageImpl(const comms_champion::Message& msg) override;\n" <<
           output::indent(1) << "virtual comms_champion::MessagePtr createInvalidMessageImpl() override;\n" <<
           output::indent(1) << "virtual comms_champion::MessagePtr createRawDataMessageImpl() override;\n" <<
           output::indent(1) << "virtual comms_champion::MessagePtr createExtraInfoMessageImpl() override;\n\n" <<
           "private:\n" <<
           output::indent(1) << "std::unique_ptr<ProtocolImpl> m_pImpl;\n"
           "};\n\n";
    common::writePluginNamespaceEnd(ns, out);
    return true;
}

bool Protocol::writeSrc(const std::string& name)
{
    auto relPath = common::pluginNamespaceNameStr() + '/' + name + common::protocolNameStr() + ".cpp";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& pluginNs = common::pluginNamespaceNameStr();
    out << "#include \"" << common::protocolNameStr() << ".h\"\n\n"
           "#include " << common::localHeader(pluginNs, common::emptyString(), common::transportFrameFileName()) << '\n' <<
           "#include " << common::localHeader(pluginNs, common::emptyString(), name + common::transportMessageNameStr() + ".h") << "\n\n" <<
           "namespace cc = comms_champion;\n\n";

    auto& ns = m_db.getProtocolNamespace();
    common::writePluginNamespaceBegin(ns, out);

    auto writeBaseFunc =
        [&out, &name](unsigned ind)
        {
        out << output::indent(ind) << "cc::ProtocolBase<\n" <<
               output::indent(ind + 1) << common::pluginNamespaceStr() << name << ",\n" <<
               output::indent(ind + 1) << common::pluginNamespaceStr() << name << common::transportMessageNameStr() << '\n' <<
               output::indent(ind) << ">";
        };

    out << "class " << common::protocolNameStr() << "Impl : public\n";
    writeBaseFunc(1);
    out << "\n{\n" <<
           output::indent(1) << "using Base =\n";
    writeBaseFunc(2);
    auto protName = m_db.getPackageName();
    out << ";\n" <<
           "public:\n" <<
           output::indent(1) << "friend class " << common::scopeFor(ns, common::pluginNamespaceStr() + common::protocolNameStr()) << ";\n\n" <<
           output::indent(1) << common::protocolNameStr() << "Impl() = default;\n" <<
           output::indent(1) << "virtual ~" << common::protocolNameStr() << "Impl() = default;\n\n" <<
           "protected:\n" <<
           output::indent(1) << "virtual const QString& nameImpl() const override\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "static const QString Str(\"" + protName + "\");\n" <<
           output::indent(2) << "return Str;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "using Base::createInvalidMessageImpl;\n" <<
           output::indent(1) << "using Base::createRawDataMessageImpl;\n" <<
           output::indent(1) << "using Base::createExtraInfoMessageImpl;" <<
           "};\n\n" <<
           common::protocolNameStr() << "::" << common::protocolNameStr() << "()\n" <<
           "  : m_pImpl(new " << common::protocolNameStr() << "Impl())\n"
           "{\n"
           "}\n\n" <<
           common::protocolNameStr() << "::~" << common::protocolNameStr() << "() = default;\n" <<
           "const QString& Protocol::nameImpl() const\n"
           "{\n" <<
           output::indent(1) << "return m_pImpl->name();\n"
           "}\n\n" <<
           common::protocolNameStr() << "::MessagesList " << common::protocolNameStr() << "::readImpl(const comms_champion::DataInfo& dataInfo, bool final)\n" <<
           "{\n" <<
           output::indent(1) << "return m_pImpl->read(dataInfo, final);\n" <<
           "}\n\n" <<
           "cc::DataInfoPtr " << common::protocolNameStr() << "::writeImpl(cc::Message& msg)\n"
           "{\n" <<
           output::indent(1) << "return m_pImpl->write(msg);\n" <<
           "}\n\n" <<
           common::protocolNameStr() << "::MessagesList " << common::protocolNameStr() << "::createAllMessagesImpl()\n"
           "{\n" <<
           output::indent(1) << "return m_pImpl->createAllMessages();\n"
           "}\n\n"
           "cc::MessagePtr " << common::protocolNameStr() << "::createMessageImpl(const QString& idAsString, unsigned idx)\n" <<
           "{\n" <<
           output::indent(1) << "return static_cast<cc::Protocol*>(m_pImpl.get())->createMessage(idAsString, idx);\n" <<
           "}\n\n" <<
           common::protocolNameStr() << "::UpdateStatus " << common::protocolNameStr() << "::updateMessageImpl(cc::Message& msg)\n" <<
           "{\n" <<
           output::indent(1) << "return m_pImpl->updateMessage(msg);\n" <<
           "}\n\n"
           "cc::MessagePtr " << common::protocolNameStr() << "::cloneMessageImpl(const cc::Message& msg)\n" <<
           "{\n" <<
           output::indent(1) << "return m_pImpl->cloneMessage(msg);\n"
           "}\n\n" <<
           "cc::MessagePtr " << common::protocolNameStr() << "::createInvalidMessageImpl()\n"
           "{\n" <<
           output::indent(1) << "return m_pImpl->createInvalidMessageImpl();\n" <<
           "}\n\n"
           "cc::MessagePtr " << common::protocolNameStr() << "::createRawDataMessageImpl()\n"
           "{\n" <<
           output::indent(1) << "return m_pImpl->createRawDataMessageImpl();\n"
           "}\n\n" <<
           "cc::MessagePtr " << common::protocolNameStr() << "::createExtraInfoMessageImpl()\n"
           "{\n" <<
           output::indent(1) << "return m_pImpl->createExtraInfoMessageImpl();\n"
           "}\n\n";

    common::writePluginNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
