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

#include "get.h"
#include "prop.h"
#include "output.h"
#include "DB.h"

namespace sbe2comms
{

namespace
{

void writeFileHeader(std::ostream& out, const std::string& name)
{
    out << "/// \\file\n"
           "/// \\brief Contains definition of " << name << " message and its fields.\n\n";
}

void writeIncludes(std::ostream& out, DB& db, const std::string& msgName)
{
    out <<
        "#pragma once\n"
        "\n"
        "#include \"comms/MessageBase.h\"\n"
        "#include \"" << get::protocolNamespace(db) << '/' << get::fieldsDefFileName() << "\"\n"
        "#include \"" << "details/" << msgName << ".h\n"
        "\n";
}

void openNamespaces(std::ostream& out, DB& db)
{
    auto& ns = get::protocolNamespace(db);
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

    auto& ns = get::protocolNamespace(db);
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
        "/// @see " << name << "\n"
        "template <typename TOpt = comms::option::EmptyOption>\n"
        "struct " << name << fieldsClassSuffix() << "\n"
        "{\n";
}

void closeFieldsDef(std::ostream& out, const std::string& name)
{
    out << "}; // " << name << fieldsClassSuffix() << "\n\n";
}

} // namespace

Message::Message(xmlNodePtr node)
  : m_node(node)
{
}

bool Message::write(const std::string& filename, DB& db)
{
    if (!createFields(db)) {
        return false;
    }

    std::ofstream stream(filename);
    if (!stream) {
        std::cerr << "ERROR: Failed to create " << filename;
        return false;
    }

    auto& msgName = name(db);
    writeFileHeader(stream, msgName);
    writeIncludes(stream, db, msgName);
    openNamespaces(stream, db);
    bool result =
        writeFields(stream, db) &&
        writeMessageClass(stream, db);
    closeNamespaces(stream, db);
    stream.flush();

    bool written = stream.good();
    if (!written) {
        std::cerr << "ERROR: Failed to write message file" << std::endl;
    }

    return result && written;
}

const std::string& Message::name(DB& db)
{
    retrieveProps(db);
    return prop::name(m_props);
}

bool Message::createFields(DB& db)
{
    if (m_fields) {
        return true;
    }

    auto& msgName = name(db);
    assert(m_node != nullptr);
    auto* child = m_node->children;
    while (child != nullptr) {
        if (child->type == XML_ELEMENT_NODE) {
            auto fieldPtr = Field::create(child, msgName);
            if (!fieldPtr) {
                std::cerr << "ERROR: Unknown field kind \"" << child->name << "\"!" << std::endl;
                return false;
            }

            if (!insertField(std::move(fieldPtr), db)) {
                return false;
            }
        }

        child = child->next;
    }

    return true;
}

bool Message::insertField(FieldPtr field, DB& db)
{
    if (!m_fields) {
        m_fields = FieldsList();
    }
    static_cast<void>(db);
    // TODO: do padding

    (*m_fields).push_back(std::move(field));
    return true;
}

bool Message::writeFields(std::ostream& out, DB& db)
{
    auto& msgName = name(db);
    openFieldsDef(out, msgName);
    assert(m_fields);
    bool result = true;
    for (auto& f : *m_fields) {
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
    for (auto& f : *m_fields) {
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
    auto& n = name(db);
    out <<
        "/// \\brief Definition of " << n << " message\n"
        "/// \\details Inherits from @b comms::MessageBase\n"
        "///     while providing @b TMsgBase as common interface class as well as\n"
        "///     various implementation options. @n\n"
        "///     See @ref " << n << fieldsClassSuffix() << " for definition of the fields this message contains\n"
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
        output::indent(1) << "/// \\details See definition of @b COMMS_MSG_FIELDS_ACCESS macro\n" <<
        output::indent(1) << "///     related to @b comms::MessageBase class from COMMS library\n" <<
        output::indent(1) << "///     for details.\n" <<
        output::indent(1) << "///     \n" <<
        output::indent(1) << "///     The field names are:\n";
    for (auto& f : *m_fields) {
        auto& p = f->props(db);
        auto& fieldName = prop::name(p);
        out << output::indent(1) <<
            "///     @li @b " << fieldName <<
            " for @ref " << n << fieldsClassSuffix() << "::" <<
            fieldName << " field.\n";
    }
    out << output::indent(1) << "COMMS_MSG_FIELDS_ACCESS(\n";
    bool firstField = true;
    for (auto& f : *m_fields) {
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

void Message::retrieveProps(DB& db)
{
    if (!m_props.empty()) {
        return;
    }

    m_props = xmlParseNodeProps(m_node, db.m_doc.get());
    assert(!m_props.empty());
}

} // namespace sbe2comms
