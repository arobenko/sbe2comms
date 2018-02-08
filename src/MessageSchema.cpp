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

#include "MessageSchema.h"

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "prop.h"
#include "DB.h"
#include "common.h"
#include "log.h"
#include "output.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

MessageSchema::MessageSchema(xmlNodePtr node, xmlDocPtr doc)
  : m_props(xmlParseNodeProps(node, doc))
{
}

const std::string& MessageSchema::package()
{
    return m_props["package"]; // may create missing node
}

unsigned MessageSchema::version() const
{
    return prop::version(m_props);
}

unsigned MessageSchema::id() const
{
    return prop::id(m_props);
}

const std::string& MessageSchema::byteOrder() const
{
    return prop::byteOrder(m_props);
}

const std::string& MessageSchema::headerType() const
{
    return prop::headerType(m_props);
}

bool MessageSchema::write(DB& db)
{
    auto& ns = db.getProtocolNamespace();
    if (!common::createProtocolDefDir(db.getRootPath(), ns)) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(db.getProtocolNamespace(), common::messageSchemaFileNameStr());
    auto filePath = bf::path(db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& msgHeaderType = db.getMessageHeaderType();
    if (msgHeaderType.empty()) {
        log::error() << "Unknown message header type." << std::endl;
        return false;                        
    }

    out << "/// \\file\n"
           "/// \\brief Contains compile time constants and types relevant to the schema.\n\n"
           "#pragma once\n\n"
           "#include \"comms/traits.h\"\n\n" 
           "#include " << common::localHeader(ns, common::fieldNamespaceNameStr(), msgHeaderType + ".h") << "\n";

    if (db.hasSimpleOpenFramingHeaderTypeDefined()) {
        out << "#include " << common::localHeader(ns, common::fieldNamespaceNameStr(), db.getSimpleOpenFramingHeaderTypeName() + ".h") << "\n";
    }
    else {
        out << "#include " << common::localHeader(ns, common::builtinNamespaceNameStr(), common::openFramingHeaderStr() + ".h") << "\n";
    }
    out << "\n";

    common::writeProtocolNamespaceBegin(ns, out);
    out << "class " << common::messageSchemaStr() << "\n"
           "{\n" <<
           output::indent(1) << "/// \\brief Endianness tag ussed by the COMMS library\n" <<
           output::indent(1) << "using Endian = comms::traits::endian::";
    if (ba::contains(db.getEndian(), "Big")) {
        out << "Big";
    }
    else {
        out << "Little";
    }
    
    out << ";\n\n" <<
           output::indent(1) << "/// \\brief Message header field type\n" <<
           output::indent(1) << "using MessageHeader = " << common::scopeFor(ns, common::fieldNamespaceStr() + db.getMessageHeaderType()) << ";\n\n" <<
           output::indent(1) << "/// \\brief Simple open framing header field type\n" <<
           output::indent(1) << "using SimpleOpenFramingHeader = ";

    if (db.hasSimpleOpenFramingHeaderTypeDefined()) {
        out << common::scopeFor(ns, common::fieldNamespaceStr() + db.getSimpleOpenFramingHeaderTypeName());
    }
    else {
        out << common::scopeFor(ns, common::builtinNamespaceStr() + common::openFramingHeaderStr());
    }
    out << ";\n\n" <<
           output::indent(1) << "/// \\brief Version of the schema\n" <<
           output::indent(1) << "static const unsigned version()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return " << db.getSchemaVersion() << "U;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) <<  "/// \\brief ID of the schema\n" <<
           output::indent(1) << "static const unsigned id()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return " << db.getSchemaId() << "U;\n" <<
           output::indent(1) << "}\n" <<
           "};\n\n";

    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
