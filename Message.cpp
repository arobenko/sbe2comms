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
#include "DB.h"
#include "BasicField.h"
#include "GroupField.h"
#include "DataField.h"

namespace sbe2comms
{

namespace
{

void writeIncludes(std::ostream& out, DB& db)
{
    out <<
        "#pragma once\n"
        "\n"
        "#include \"comms/MessageBase.h\"\n"
        "#include \"" << get::protocolNamespace(db) << '/' << get::fieldsDefFileName() << "\"\n"
        "\n";
}

void openNamespaces(std::ostream& out, DB& db)
{
    auto& ns = get::protocolNamespace(db);
    if (!ns.empty()) {
        out << "namespace " << ns << "\n"
               "{\n";
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

    writeIncludes(stream, db);
    openNamespaces(stream, db);
    bool result =
        writeFields(stream, db);
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
    auto iter = m_props.find(get::nameProperty());
    assert(iter != m_props.end());
    return iter->second;
}

bool Message::createFields(DB& db)
{
    if (m_fields) {
        return true;
    }

    using FieldCreateFunc = std::function<FieldPtr (xmlNodePtr)>;
    static const std::map<std::string, FieldCreateFunc> CreateMap = {
        std::make_pair(
            "field",
            [](xmlNodePtr n)
            {
                return FieldPtr(new BasicField(n));
            }),
        std::make_pair(
            "group",
            [](xmlNodePtr n)
            {
                return FieldPtr(new GroupField(n));
            }),
        std::make_pair(
            "data",
            [](xmlNodePtr n)
            {
                return FieldPtr(new DataField(n));
            })
    };

    assert(m_node != nullptr);
    auto* child = m_node->children;
    while (child != nullptr) {
        if (child->type == XML_ELEMENT_NODE) {
            std::string name(reinterpret_cast<const char*>(child->name));
            auto iter = CreateMap.find(name);
            if (iter == CreateMap.end()) {
                std::cerr << "ERROR: Unknown field type \"" << name << "\"!" << std::endl;
                return false;
            }

            auto fieldPtr = iter->second(child);
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
    static_cast<void>(field);
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
    closeFieldsDef(out, msgName);
    return result;
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
