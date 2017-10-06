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

#include "Message.h"

#include <iostream>
#include <fstream>
#include <functional>

#include <boost/filesystem.hpp>

#include "get.h"
#include "prop.h"
#include "output.h"
#include "DB.h"
#include "log.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

namespace
{

void writeFileHeader(std::ostream& out, const std::string& name)
{
    out << "/// \\file\n"
           "/// \\brief Contains definition of " << name << " message and its fields.\n\n"
           "#pragma once\n\n";
}

void writeIncludes(std::ostream& out, DB& db, const std::string& msgName)
{
    out <<
        "#include \"comms/MessageBase.h\"\n"
        "#include \"" << db.getProtocolNamespace() << '/' << get::fieldsDefFileName() << "\"\n"
        "#include \"" << "details/" << msgName << ".h\"\n"
        "\n";
}

void openNamespaces(std::ostream& out, DB& db)
{
    auto& ns = db.getProtocolNamespace();
    if (!ns.empty()) {
        out << "namespace " << ns << "\n"
               "{\n"
               "\n";
    }

    out << "namespace " << get::messageDirName() << "\n"
            "{\n"
            "\n";
}

void closeNamespaces(std::ostream& out, DB& db)
{
    out << "} // namespace " << get::messageDirName() << "\n"
            "\n";

    auto& ns = db.getProtocolNamespace();
    if (!ns.empty()) {
        out << "} // namespace " << ns << "\n"
               "\n";
    }
}

const std::string& fieldsClassSuffix()
{
    static const std::string Suffix("Fields");
    return Suffix;
}


void openFieldsDef(std::ostream& out, const std::string& name)
{
    out <<
        "/// \\brief Accumulates details of all the " << name << " message fields.\n"
        "/// \\tparam TOpt Extra options to be passed to all fields.\n"
        "/// \\see " << name << "\n"
        "template <typename TOpt = comms::option::EmptyOption>\n"
        "struct " << name << fieldsClassSuffix() << "\n"
        "{\n";
}

void closeFieldsDef(std::ostream& out, const std::string& name)
{
    out << "}; // " << name << fieldsClassSuffix() << "\n\n";
}

} // namespace

Message::Message(DB& db, xmlNodePtr node)
  : m_db(db),
    m_node(node)
{
}

bool Message::parse()
{
    m_props = xmlParseNodeProps(m_node, m_db.getDoc());
    if (getName().empty()) {
        return false;
    }

    if (!createFields()) {
        return false;
    }

    return true;
}

bool Message::write(DB& db)
{
    bf::path root(db.getRootPath());
    bf::path protocolRelDir(db.getProtocolRelDir());
    bf::path messagesDir(root / protocolRelDir / get::messageDirName());

    boost::system::error_code ec;
    bf::create_directories(messagesDir, ec);
    if (ec) {
        std::cerr << "ERROR: Failed to create \"" << messagesDir.string() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }

    const std::string Ext(".h");
    auto filename = getName() + Ext;
    auto relPath = protocolRelDir / get::messageDirName() / filename;
    auto filePath = messagesDir / filename;

    std::cout << "INFO: Generating " << relPath.string() << std::endl;
    return writeMessageDef(filePath.string(), db);
}

const std::string& Message::getName() const
{
    assert(!m_props.empty());
    return prop::name(m_props);
}

bool Message::createFields()
{
    assert(m_fields.empty());
    auto children = xmlChildren(m_node);
    for (auto c : children) {
        auto fieldPtr = Field::create(m_db, c, getName());
        if (!fieldPtr) {
            log::error() << "Unknown field kind \"" << c->name << "\"!" << std::endl;
            return false;
        }

        if (!fieldPtr->parse()) {
            return false;
        }

        if (!fieldPtr->doesExist()) {
            continue;
        }

        m_fields.push_back(std::move(fieldPtr));
    }
    return true;
}

bool Message::writeFields(std::ostream& out, DB& db)
{
    auto& msgName = getName();
    openFieldsDef(out, msgName);
    bool result = true;
    for (auto& f : m_fields) {
        result = f->write(out, db, 1) && result;
    }

    result = writeAllFieldsDef(out, db) && result;
    closeFieldsDef(out, msgName);
    return result;
}

bool Message::writeAllFieldsDef(std::ostream& out, DB& db)
{
    out << output::indent(1) <<
        "/// \\brief All the fields bundled in std::tuple.\n" <<
        output::indent(1) <<
        "using All = std::tuple<\n";

    bool first = true;
    for (auto& f : m_fields) {
        if (!first) {
            out << ",\n";
        }
        else {
            first = false;
        }

        auto& p = f->props(db);
        out << output::indent(2) << prop::name(p);
    }
    out << '\n' << output::indent(1) << ">;\n\n";
    return true;
}

bool Message::writeMessageClass(std::ostream& out, DB& db)
{
    auto& n = getName();
    out <<
        "/// \\brief Definition of " << n << " message\n"
        "/// \\details Inherits from \\b comms::MessageBase\n"
        "///     while providing \\b TMsgBase as common interface class as well as\n"
        "///     various implementation options. \\n\n"
        "///     See \\ref " << n << fieldsClassSuffix() << " for definition of the fields this message contains\n"
        "///         and COMMS_MSG_FIELDS_ACCESS() for fields access details.\n"
        "/// \\tparam TMsgBase Common interface class for all the messages.\n"
        "/// \\tparam TOpt Extra options to be passed to all fields.\n"
        "template <typename TMsgBase, typename TOpt = comms::option::EmptyOption>\n"
        "class " << n << " : public\n" <<
        output::indent(1) << "comms::MessageBase<\n" <<
        output::indent(2) << "TMsgBase,\n" <<
        output::indent(2) << "comms::option::StaticNumIdImpl<MsgId_" << n << ">,\n" <<
        output::indent(2) << "comms::option::FieldsImpl<typename " << n << fieldsClassSuffix() << "<TOpt>::All>,\n" <<
        output::indent(2) << "comms::option::MsgType<" << n << "<TMsgBase, TOpt> >\n" <<
        output::indent(1) << ">\n"
        "{\n"
        "public:\n" <<
        output::indent(1) << "/// \\brief Allow access to internal fields.\n" <<
        output::indent(1) << "/// \\details See definition of \\b COMMS_MSG_FIELDS_ACCESS macro\n" <<
        output::indent(1) << "///     related to \\b comms::MessageBase class from COMMS library\n" <<
        output::indent(1) << "///     for details.\n" <<
        output::indent(1) << "///     \n" <<
        output::indent(1) << "///     The field names are:\n";
    for (auto& f : m_fields) {
        auto& p = f->props(db);
        auto& fieldName = prop::name(p);
        out << output::indent(1) <<
            "///     \\li \\b " << fieldName <<
            " for \\ref " << n << fieldsClassSuffix() << "::" <<
            fieldName << " field.\n";
    }
    out << output::indent(1) << "COMMS_MSG_FIELDS_ACCESS(\n";
    bool firstField = true;
    for (auto& f : m_fields) {
        if (!firstField) {
            out << ",\n";
        }
        else {
            firstField = false;
        }
        auto& p = f->props(db);
        auto& fieldName = prop::name(p);
        out << output::indent(2) << fieldName;
    }
    out << '\n' << output::indent(1) << ");\n";
    out << "};\n\n";

    return true;
}

bool Message::writeMessageDef(const std::string& filename, DB& db)
{
    std::ofstream stream(filename);
    if (!stream) {
        log::error() << "Failed to create " << filename;
        return false;
    }

    auto& msgName = getName();
    writeFileHeader(stream, msgName);
    writeExtraDefHeaders(stream);
    writeIncludes(stream, db, msgName);
    openNamespaces(stream, db);
    bool result =
        writeFields(stream, db) &&
        writeMessageClass(stream, db);
    closeNamespaces(stream, db);
    stream.flush();

    bool written = stream.good();
    if (!written) {
        log::error() << "Failed to write message file" << std::endl;
    }

    return result && written;
}

void Message::writeExtraDefHeaders(std::ostream& out)
{
    std::set<std::string> extraHeaders;
    for (auto& f : m_fields) {
        f->updateExtraHeaders(extraHeaders);
    }

    if (extraHeaders.empty()) {
        return;
    }

    for (auto& h : extraHeaders) {
        out << "#include " << h << '\n';
    }
    out << '\n';
}

} // namespace sbe2comms
