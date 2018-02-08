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

#include "TransportMessage.h"

#include <fstream>
#include <boost/filesystem.hpp>

#include "DB.h"
#include "common.h"
#include "log.h"
#include "output.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

namespace
{

const std::string CreateFieldPropsFuncPrefix("createFieldProps_");

void writeMessageHeaderFunc(std::ostream& out, const std::string& name)
{
    out << "QVariantMap " << CreateFieldPropsFuncPrefix << name << "()\n"
           "{\n" <<
           output::indent(1) << "return " << common::fieldNamespaceStr() << "createProps_" << name << "(\"Message Header\");\n" <<
           "}\n";
}

void writeDataFunc(std::ostream& out)
{
    out << "QVariantMap " << CreateFieldPropsFuncPrefix << "data" << "()\n"
           "{\n" <<
           output::indent(1) << "return comms_champion::property::field::ArrayList().name(\"Data\").asMap();\n" <<
           "}\n";
}

} // namespace

bool TransportMessage::write()
{
    return
        writePluginHeader(common::messageHeaderFrameStr()) &&
        writeMessageHeaderSrc() &&
        writePluginHeader(common::openFramingHeaderFrameStr()) &&
        writeOpenFramingHeaderSrc();
}

bool TransportMessage::writePluginHeader(const std::string& name)
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    auto relPath = common::pluginNamespaceNameStr() + '/' + name + common::transportMessageNameStr() + ".h";
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
           "#include \"comms_champion/comms_champion.h\"\n\n"
           "#include " << common::localHeader(ns, common::transportFrameFileName()) << '\n' <<
           "#include " << common::localHeader(pluginNs, common::msgInterfaceFileName()) << '\n' <<
           "#include " << common::localHeader(pluginNs, common::transportFrameFileName()) << "\n\n";

    common::writePluginNamespaceBegin(ns, out);

    out << "class " << name << common::transportMessageNameStr() << " : public\n" <<
           output::indent(1) << "comms_champion::TransportMessageBase<\n" <<
           output::indent(2) << common::scopeFor(pluginNs, common::msgInterfaceStr()) << "<>,\n" <<
           output::indent(2) << common::scopeFor(pluginNs, name + "::AllFields") << '\n' <<
           output::indent(1) << ">\n" <<
           "{\n"
           "protected:\n" <<
           output::indent(1) << "virtual const QVariantList& fieldsPropertiesImpl() const override;\n" <<
           "};\n\n";

    common::writePluginNamespaceEnd(ns, out);
    return true;
}

bool TransportMessage::writeMessageHeaderSrc()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    auto relPath = common::pluginNamespaceNameStr() + '/' + common::messageHeaderFrameStr() + common::transportMessageNameStr() + ".cpp";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& ns = m_db.getProtocolNamespace();
    auto& pluginNs = common::pluginNamespaceNameStr();

    out << "#include \"" << common::messageHeaderFrameStr() << common::transportMessageNameStr() << ".h\"\n\n"
           "#include <cassert>\n\n"
           "#include " << common::localHeader(pluginNs, common::fieldHeaderFileName()) << "\n\n";


    common::writePluginNamespaceBegin(ns, out);
    out << "namespace\n"
           "{\n\n";

    writeMessageHeaderFunc(out, m_db.getMessageHeaderType());
    out << '\n';
    writeDataFunc(out);
    out << "\n"
           "QVariantList createFieldsProperties()\n"
           "{\n" <<
           output::indent(1) << "QVariantList props;\n" <<
           output::indent(1) << "props.append(" << CreateFieldPropsFuncPrefix << m_db.getMessageHeaderType() << "());\n" <<
           output::indent(1) << "props.append(" << CreateFieldPropsFuncPrefix << "data());\n\n" <<
           output::indent(1) << "return props;\n"
           "}\n\n"
           "} // namespace \n\n"
           "const QVariantList& " << common::messageHeaderFrameStr() << common::transportMessageNameStr() << "::fieldsPropertiesImpl() const\n"
           "{\n" <<
           output::indent(1) << "static const auto Props = createFieldsProperties();\n" <<
           output::indent(1) << "return Props;\n" <<
           "}\n\n";
    common::writePluginNamespaceEnd(ns, out);
    return true;
}

bool TransportMessage::writeOpenFramingHeaderSrc()
{
    if (!common::createPluginDefDir(m_db.getRootPath())) {
        return false;
    }

    auto relPath = common::pluginNamespaceNameStr() + '/' + common::openFramingHeaderFrameStr() + common::transportMessageNameStr() + ".cpp";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& ns = m_db.getProtocolNamespace();
    auto& pluginNs = common::pluginNamespaceNameStr();

    bool hasOpenFramingTypeDefined = m_db.hasSimpleOpenFramingHeaderTypeDefined();
    auto name = common::openFramingHeaderStr();
    if (hasOpenFramingTypeDefined) {
        name = m_db.getSimpleOpenFramingHeaderTypeName();;
        assert(!name.empty());
    }

    out << "#include \"" << common::openFramingHeaderFrameStr() << common::transportMessageNameStr() << ".h\"\n\n"
           "#include <cassert>\n\n"
           "#include " << common::localHeader(pluginNs, common::fieldHeaderFileName()) << "\n";

    if (!hasOpenFramingTypeDefined) {
        out << "#include " << common::localHeader(ns, common::builtinNamespaceNameStr(), common::openFramingHeaderStr() + ".h") << '\n';
    }
    out << '\n';

    common::writePluginNamespaceBegin(ns, out);
    out << "namespace\n"
           "{\n\n";

    out << "QVariantMap " << CreateFieldPropsFuncPrefix << name << "()\n"
           "{\n";
    if (hasOpenFramingTypeDefined) {
        out << output::indent(1) << "return " << common::fieldNamespaceStr() << "createProps_" << name << "(\"Open Framing Header\");\n";
    }
    else {
        out << output::indent(1) << "using Field = " << common::scopeFor(common::emptyString(), common::builtinNamespaceStr() + name) << ";\n" <<
               output::indent(1) << "return \n" <<
               output::indent(2) << "comms_champion::property::field::ForField<Field>()\n" <<
               output::indent(3) << ".name(\"Open Framing Header\")\n" <<
               output::indent(3) << ".add(\n" <<
               output::indent(4) << "comms_champion::property::field::IntValue()\n" <<
               output::indent(5) << ".name(\"messageLength\")\n" <<
               output::indent(5) << ".displayOffset(6)\n" <<
               output::indent(5) << ".asMap())\n" <<
               output::indent(3) << ".add(\n" <<
               output::indent(4) << "comms_champion::property::field::IntValue()\n" <<
               output::indent(5) << ".name(\"encodingType\")\n" <<
               output::indent(5) << ".readOnly()\n" <<
               output::indent(5) << ".asMap())\n" <<
               output::indent(3) << ".asMap();\n";
    }
    out << "}\n\n";

    writeMessageHeaderFunc(out, m_db.getMessageHeaderType());
    out << '\n';
    writeDataFunc(out);
    out << "\n"
           "QVariantList createFieldsProperties()\n"
           "{\n" <<
           output::indent(1) << "QVariantList props;\n" <<
           output::indent(1) << "props.append(" << CreateFieldPropsFuncPrefix << name << "());\n" <<
           output::indent(1) << "props.append(" << CreateFieldPropsFuncPrefix << m_db.getMessageHeaderType() << "());\n" <<
           output::indent(1) << "props.append(" << CreateFieldPropsFuncPrefix << "data());\n\n" <<
           output::indent(1) << "return props;\n"
           "}\n\n"
           "} // namespace \n\n"
           "const QVariantList& " << common::openFramingHeaderFrameStr() << common::transportMessageNameStr() << "::fieldsPropertiesImpl() const\n"
           "{\n" <<
           output::indent(1) << "static const auto Props = createFieldsProperties();\n" <<
           output::indent(1) << "return Props;\n" <<
           "}\n\n";
    common::writePluginNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
