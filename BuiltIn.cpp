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

#include "BuiltIn.h"

#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>

#include "output.h"
#include "DB.h"

namespace bf = boost::filesystem;

void writeBuiltInInt(std::ostream& out, const std::string& name)
{
    out << "/// \\brief Definition of built-in \"" << name << "\" type\n"
           "/// \\tparam TFieldBase Base class of the field type.\n"
           "/// \\tparam TOpt Extra options from \\b comms::option namespace \n"
           "template <typename TFieldBase, typename... TOpt>\n"
           "using " << name << " = \n"
           "    comms::field::IntValue<\n"
           "        TFieldBase,\n"
           "        std::" << name << "_t,\n"
           "        TOpt...\n"
           "    >;\n\n";
}

void writeBuiltInFloat(std::ostream& out, const std::string& name)
{
    out << "/// \\brief Definition of built-in \"" << name << "\" type\n"
           "/// \\tparam TFieldBase Base class of the field type.\n"
           "/// \\tparam TOpt Extra options from \\b comms::option namespace \n"
           "template <typename TFieldBase, typename... TOpt>\n"
           "using " << name << "Field = \n"
           "    comms::field::FloatValue<\n"
           "        TFieldBase,\n"
           "        " << name << ",\n"
           "        TOpt...\n"
           "    >;\n\n";
}


namespace sbe2comms
{

bool BuiltIn::write(DB& db)
{
    bf::path root(db.getRootPath());
    bf::path protocolRelDir(db.getProtocolRelDir());

    boost::system::error_code ec;
    bf::create_directories(protocolRelDir, ec);
    if (ec) {
        std::cerr << "ERROR: Failed to create \"" << protocolRelDir.string() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }

    auto relPath = protocolRelDir / "sbe2comms.h";
    auto filePath = root / relPath;
    std::cout << "INFO: Generating " << relPath.string() << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        std::cerr << "ERROR: Failed to create " << relPath.string() << std::endl;
        return false;
    }

    out << "/// \\file\n"
           "/// \\brief Contains definition of built-in types and helper classes\n"
           "\n\n"
           "#pragma once\n\n"
           "#include <cstdint>"
           "#include \"comms/fields.h\"\n\n"
           "namespace sbe2comms\n"
           "{\n\n";
    writeBuiltInInt(out, "int8");
    writeBuiltInInt(out, "uint8");
    writeBuiltInInt(out, "int16");
    writeBuiltInInt(out, "uint16");
    writeBuiltInInt(out, "int32");
    writeBuiltInInt(out, "uint32");
    writeBuiltInInt(out, "int64");
    writeBuiltInInt(out, "uint64");
    writeBuiltInFloat(out, "float");
    writeBuiltInFloat(out, "double");

    out << "/// \\brief Definition of \"groupList\" type.\n"
           "/// \\details Used to define \"group\" lists."
           "template <\n"
           "    typename TFieldBase,\n"
           "    typename TGroupSize,\n"
           "    typename TElement,\n"
           "    typename... TOpt>\n"
           "struct groupList : public \n"
           "    comms::field::Bundle<\n"
           "        TFieldBase,\n"
           "        std::tuple<\n"
           "            TGroupSize,\n"
           "            std::ArrayList<\n"
           "                TFieldBase,\n"
           "                TElement\n"
           "            >\n"
           "        >\n"
           "    >\n"
           "{\n"
           "    /// \\brief Defining custom read operation\n"
           "    template <typename TIter>\n"
           "    comms::ErrorStatus read(TIter& iter, std::size_t len)\n"
           "    {\n"
           "    }\n"
           "};\n\n";

    out << "}\n\n";
    return out.good();
}

} // namespace sbe2comms
