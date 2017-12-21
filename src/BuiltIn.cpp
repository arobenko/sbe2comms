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
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "output.h"
#include "DB.h"
#include "common.h"
#include "log.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{


void writeBuiltInBigUnsignedInt(std::ostream& out, const std::string& name)
{
    auto type = "std::" + name + "_t";
    auto maxVal = common::intBigUnsignedMaxValue();
    assert(maxVal.second);

    out << "/// \\brief Definition of built-in \"" << name << "\" type\n"
           "/// \\tparam TFieldBase Base class of the field type.\n"
           "/// \\tparam TOpt Extra options from \\b comms::option namespace \n"
           "template <typename TFieldBase, typename... TOpt>\n"
           "struct " << common::renameKeyword(name) << " : public\n" <<
           output::indent(1) << "comms::field::IntValue<\n" <<
           output::indent(2) << "TFieldBase,\n" <<
           output::indent(2) << type << ",\n" <<
           output::indent(2) << "TOpt...,\n" <<
           output::indent(2) << "comms::option::ValidBigUnsignedNumValueRange<0, " << common::num(maxVal.first) << ">\n" <<
           output::indent(1) << ">\n" <<
           "{\n";
    common::writeDefaultSetVersionFunc(out, 1);
    out << "};\n\n";
}

void writeBuiltInRegularInt(std::ostream& out, const std::string& name)
{
    auto minVal = common::intMinValue(name);
    auto maxVal = common::intMaxValue(name);
    assert(minVal.second);
    assert(maxVal.second);
    auto type = name;
    if (name != common::charType()) {
        type = "std::" + name + "_t";
    }

    out << "/// \\brief Definition of built-in \"" << name << "\" type\n"
           "/// \\tparam TFieldBase Base class of the field type.\n"
           "/// \\tparam TOpt Extra options from \\b comms::option namespace \n"
           "template <typename TFieldBase, typename... TOpt>\n"
           "struct " << common::renameKeyword(name) << " : public\n" <<
           output::indent(1) << "comms::field::IntValue<\n" <<
           output::indent(2) << "TFieldBase,\n" <<
           output::indent(2) << type << ",\n" <<
           output::indent(2) << "TOpt...,\n";
    if ((0 < minVal.first) ||
        (maxVal.first < 0)) {
        auto defValue = std::min(std::max(std::intmax_t(0), minVal.first), maxVal.first);
        out << output::indent(2) << "comms::option::DefaultNumValue<" << common::num(defValue) << ">,\n";
    }
    out << output::indent(2) << "comms::option::ValidNumValueRange<" << common::num(minVal.first) << ", " << common::num(maxVal.first) << ">\n" <<
           output::indent(1) << ">\n" <<
           "{\n";
    common::writeDefaultSetVersionFunc(out, 1);
    out << "};\n\n";
}

void writeBuiltInInt(std::ostream& out, const std::string& name)
{
    auto& refName = common::renameKeyword(name);
    out << "/// \\file\n"
           "/// \\brief Contains definition of implicitly defined \\ref " << common::builtinNamespaceStr() << refName << "\n"
           "\n\n"
           "#pragma once\n\n"
           "#include <cstdint>\n"
           "#include \"comms/Field.h\"\n"
           "#include \"comms/field/IntValue.h\"\n"
           "#include \"comms/options.h\"\n\n"
           "namespace " << common::builtinNamespaceNameStr() << "\n"
           "{\n\n";


    if (name == common::uint64Type()) {
        writeBuiltInBigUnsignedInt(out, name);
    }
    else {
        writeBuiltInRegularInt(out, name);
    }

    out << "} // namespace " << common::builtinNamespaceNameStr() << "\n\n";
}

void writeBuiltInFloat(std::ostream& out, const std::string& name)
{
    auto& refName = common::renameKeyword(name);
    out << "/// \\file\n"
           "/// \\brief Contains definition of implicitly defined \\ref " << common::builtinNamespaceStr() << refName << "\n"
           "\n\n"
           "#pragma once\n\n"
           "#include \"comms/Field.h\"\n"
           "#include \"comms/field/FloatValue.h\"\n"
           "#include \"comms/options.h\"\n\n"
           "namespace " << common::builtinNamespaceNameStr() << "\n"
           "{\n\n"
           "/// \\brief Definition of built-in \"" << name << "\" type\n"
           "/// \\tparam TFieldBase Base class of the field type.\n"
           "/// \\tparam TOpt Extra options from \\b comms::option namespace \n"
           "template <typename TFieldBase, typename... TOpt>\n"
           "struct " << refName << " : public\n" <<
           output::indent(1) << "comms::field::FloatValue<\n" <<
           output::indent(2) << "TFieldBase,\n" <<
           output::indent(2) << name << ",\n" <<
           output::indent(2) << "TOpt...\n" <<
           output::indent(1) << ">\n" <<
           "{\n";
    common::writeDefaultSetVersionFunc(out, 1);
    out << "};\n\n"
           "} // namespace " << common::builtinNamespaceNameStr() << "\n\n";
}

bool writeBuiltIn(DB& db, const std::string& name)
{
    if (!common::createProtocolDefDir(db.getRootPath(), db.getProtocolNamespace(), common::builtinNamespaceNameStr())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(db.getProtocolNamespace(), common::builtinNamespaceNameStr() + '/' + name + ".h");
    auto filePath = bf::path(db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    static const std::string FloatTypes[] = {
        "float",
        "double"
    };

    auto iter = std::find(std::begin(FloatTypes), std::end(FloatTypes), name);
    if (iter != std::end(FloatTypes)) {
        writeBuiltInFloat(out, name);
        return out.good();
    }
    writeBuiltInInt(out, name);
    return out.good();
}

bool writeGroupList(DB& db)
{
    if (!common::createProtocolDefDir(db.getRootPath(), db.getProtocolNamespace(), common::builtinNamespaceNameStr())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(db.getProtocolNamespace(), common::builtinNamespaceNameStr() + '/' + common::groupListStr() + ".h");
    auto filePath = bf::path(db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "/// \\file\n"
           "/// \\brief Contains definition of implicitly defined \\ref " << common::builtinNamespaceStr() << common::groupListStr() << "\n"
           "\n\n"
           "#pragma once\n\n"
           "#include <cstdint>\n"
           "#include <type_traits>\n\n"
           "#include \"comms/Field.h\"\n"
           "#include \"comms/field/ArrayList.h\"\n"
           "#include \"comms/field/Optional.h\"\n"
           "#include \"comms/options.h\"\n"
           "#include \"comms/util/Tuple.h\"\n\n"
           "#include \"VersionSetter.h\"\n\n"
           "namespace " << common::builtinNamespaceNameStr() << "\n"
           "{\n\n"
           "/// \\brief Generic list type to be used to defaine a \"group\" list.\n"
           "/// \\tparam TFieldBase Common base class of all the fields.\n"
           "/// \\tparam TElement Element of the list, expected to be a variant of \\b comms::field::Bundle.\n"
           "/// \\tparam TDimensionType Dimention type field with \"blockLength\" and \"numInGroup\" members.\n"
           "/// \\tparam TRootCount Number of root block fields in the element.\n"
           "/// \\tparam TOpt Extra options for the list class.\n"
           "template <\n" <<
           output::indent(1) << "typename TFieldBase,\n" <<
           output::indent(1) << "typename TElement,\n" <<
           output::indent(1) << "typename TDimensionType,\n" <<
           output::indent(1) << "std::size_t TRootCount,\n" <<
           output::indent(1) << "typename... TOpt\n" <<
           ">\n"
           "struct " << common::groupListStr() << " : public\n" <<
           output::indent(1) << "comms::field::ArrayList<\n" <<
           output::indent(2) << "TFieldBase,\n" <<
           output::indent(2) << "TElement,\n" <<
           output::indent(2) << "TOpt...\n" <<
           output::indent(1) << ">\n" <<
           "{\n" <<
           output::indent(1) << "/// \\brief Get length of serialised data.\n" <<
           output::indent(1) << "constexpr std::size_t length() const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << common::fieldBaseDefStr() <<
           output::indent(2) << "return TDimensionType::maxLength() + Base::length();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Read field value from input data sequence.\n" <<
           output::indent(1) << "template <typename TIter>\n" <<
           output::indent(1) << "comms::ErrorStatus read(TIter& iter, std::size_t len)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "TDimensionType dimType;\n" <<
           output::indent(2) << "auto es = dimType.read(iter, len);\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "auto count = dimType.field_numInGroup().value();\n" <<
           output::indent(2) << "auto remLen = len - dimType.length();\n" <<
           output::indent(2) << "for (decltype(count) idx = 0; idx < count; ++idx) {\n" <<
           output::indent(3) << "using IterType = typename std::decay<decltype(iter)>::type;\n" <<
           output::indent(3) << "using IterCategory = typename std::iterator_traits<IterType>::iterator_category;\n" <<
           output::indent(3) << "static_assert(\n" <<
           output::indent(4) << "std::is_copy_constructible<IterType>::value &&\n" <<
           output::indent(4) << "std::is_base_of<std::forward_iterator_tag, IterCategory>::value,\n" <<
           output::indent(4) << "\"Used iterator type is not supported for read operation\");\n" <<
           output::indent(3) << "IterType iterTmp(iter);\n" <<
           output::indent(3) << "auto blockLength = static_cast<std::size_t>(dimType.field_blockLength().value());\n" <<
           output::indent(3) << "if (remLen < blockLength) {\n" <<
           output::indent(4) << "return comms::ErrorStatus::NotEnoughData;\n" <<
           output::indent(3) << "}\n\n" <<
           output::indent(3) << common::fieldBaseDefStr() <<
           output::indent(3) << "Base::value().emplace_back();\n" <<
           output::indent(3) << "auto& lastElem = Base::value().back();\n" <<
           output::indent(3) << "comms::util::tupleAccumulate(lastElem.value(), false, VersionSetter(m_version));\n" <<
           output::indent(3) << "es = lastElem.template readUntil<TRootCount>(iterTmp, blockLength);\n" <<
           output::indent(3) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(4) << "Base::value().pop_back();\n" <<
           output::indent(4) << "return es;\n" <<
           output::indent(3) << "}\n\n" <<
           output::indent(3) << "std::advance(iter, blockLength);\n" <<
           output::indent(3) << "remLen -= blockLength;\n\n" <<
           output::indent(3) << "es = lastElem.template readFrom<TRootCount>(iter, remLen);\n" <<
           output::indent(3) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(4) << "Base::value().pop_back();\n" <<
           output::indent(4) << "return es;\n" <<
           output::indent(3) << "}\n\n" <<
           output::indent(3) << "remLen -= Base::value().back().template lengthFrom<TRootCount>();\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "return checkFailOnInvalid();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Read field value from input data sequence without error check and status report.\n" <<
           output::indent(1) << "template <typename TIter>\n" <<
           output::indent(1) << "void readNoStatus(TIter& iter) = delete; // not supported\n\n" <<
           output::indent(1) << "/// \\brief Write current field value to output data sequence.\n" <<
           output::indent(1) << "template <typename TIter>\n" <<
           output::indent(1) << "comms::ErrorStatus write(TIter& iter, std::size_t len) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "if (len < length()) {\n" <<
           output::indent(3) << "return comms::ErrorStatus::BufferOverflow;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "writeNoStatus(iter);\n" <<
           output::indent(2) << "return comms::ErrorStatus::Success;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Write current field value to output data sequence  without error check and status report.\n" <<
           output::indent(1) << "template <typename TIter>\n" <<
           output::indent(1) << "void writeNoStatus(TIter& iter) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << common::fieldBaseDefStr() <<
           output::indent(2) << "TDimensionType dimType;\n" <<
           output::indent(2) << "auto& blockLengthVal = dimType.field_blockLength().value();\n" <<
           output::indent(2) << "using BlockLengthValType = typename std::decay<decltype(blockLengthVal)>::type;\n" <<
           output::indent(2) << "blockLengthVal = static_cast<BlockLengthValType>(TElement::template maxLengthUntil<TRootCount>());\n\n" <<
           output::indent(2) << "auto& numInGroupVal = dimType.field_numInGroup().value();\n" <<
           output::indent(2) << "using NumInGroupValType = typename std::decay<decltype(numInGroupVal)>::type;\n" <<
           output::indent(2) << "numInGroupVal = static_cast<NumInGroupValType>(Base::value().size());\n" <<
           output::indent(2) << "dimType.writeNoStatus(iter);\n\n" <<
           output::indent(2) << "Base::writeNoStatus(iter);\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Check validity of the field value.\n" <<
           output::indent(1) << "bool valid() const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << common::fieldBaseDefStr() <<
           output::indent(2) << "TDimensionType dimType;\n" <<
           output::indent(2) << "auto& blockLengthVal = dimType.field_blockLength().value();\n" <<
           output::indent(2) << "using BlockLengthValType = typename std::decay<decltype(blockLengthVal)>::type;\n" <<
           output::indent(2) << "blockLengthVal = static_cast<BlockLengthValType>(TElement::template maxLengthUntil<TRootCount>());\n\n" <<
           output::indent(2) << "auto& numInGroupVal = dimType.field_numInGroup().value();\n" <<
           output::indent(2) << "using NumInGroupValType = typename std::decay<decltype(numInGroupVal)>::type;\n" <<
           output::indent(2) << "numInGroupVal = static_cast<NumInGroupValType>(Base::value().size());\n" <<
           output::indent(2) << "return Base::valid() && dimType.valid();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Get minimal length that is required to serialise field of this type.\n" <<
           output::indent(1) << "static constexpr std::size_t minLength()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return TDimensionType::minLength();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "bool setVersion(unsigned value)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "m_version = value;\n\n" <<
           output::indent(2) << common::fieldBaseDefStr() <<
           output::indent(2) << "auto& list = Base::value();\n" <<
           output::indent(2) << "bool updated = false;\n" <<
           output::indent(2) << "for (auto& elem : list) {\n" <<
           output::indent(3) << "updated = comms::util::tupleAccumulate(elem.value(), updated, VersionSetter(m_version));\n" <<
           output::indent(2) << "}\n" <<
           output::indent(2) << "return updated;\n" <<
           output::indent(1) << "}\n\n" <<
           "private:\n" <<
           output::indent(1) << "struct NoFailOnInvalidTag {};\n" <<
           output::indent(1) << "struct FailOnInvalidTag {};\n\n" <<
           output::indent(1) << "comms::ErrorStatus checkFailOnInvalid() const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << common::fieldBaseDefStr() <<
           output::indent(2) << "static_assert(!Base::ParsedOptions::HasFailOnInvalid,\n" <<
           output::indent(3) << "\"comms::option::IgnoreInvalid option is not supported for \\\"groupList\\\"\");\n" <<
           output::indent(2) << "using Tag = typename std::conditional<\n" <<
           output::indent(3) << "Base::ParsedOptions::HasFailOnInvalid,\n" <<
           output::indent(3) << "FailOnInvalidTag,\n" <<
           output::indent(3) << "NoFailOnInvalidTag>::type;\n" <<
           output::indent(2) << "return checkFailOnInvalid(Tag());\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "static comms::ErrorStatus checkFailOnInvalid(NoFailOnInvalidTag)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return comms::ErrorStatus::Success;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "comms::ErrorStatus checkFailOnInvalid(FailOnInvalidTag) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << common::fieldBaseDefStr() <<
           output::indent(2) << "if (!valid()) {\n" <<
           output::indent(3) << "return Base::ParsedOptions::FailOnInvalidStatus;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "return comms::ErrorStatus::Success;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "unsigned m_version = " << db.getSchemaVersion() << ";\n"
           "};\n\n"
           "} // namespace " << common::builtinNamespaceNameStr() << "\n\n";
    return out.good();
}

bool writeOpenFrameHeader(DB& db)
{
    if (!common::createProtocolDefDir(db.getRootPath(), db.getProtocolNamespace(), common::builtinNamespaceNameStr())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(db.getProtocolNamespace(), common::builtinNamespaceNameStr() + '/' + common::openFramingHeaderStr() + ".h");
    auto filePath = bf::path(db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    static const std::string BigEndianStr("comms::Field<comms::option::BigEndian>");
    std::string sync("0x5be0");
    if (ba::ends_with(db.getEndian(), "LittleEndian")) {
        sync = "0xeb50";
    }
    out << "/// \\file\n"
           "/// \\brief Contains definition of implicitly defined \\ref " << common::builtinNamespaceStr() << common::openFramingHeaderStr() << "\n"
           "\n\n"
           "#pragma once\n\n"
           "#include <cstdint>\n\n"
           "#include \"comms/Field.h\"\n"
           "#include \"comms/field/Bundle.h\"\n"
           "#include \"comms/field/IntValue.h\"\n"
           "#include \"comms/options.h\"\n\n"
           "namespace " << common::builtinNamespaceNameStr() << "\n"
           "{\n\n"
           "/// \\brief Simple Open Framing Header definition.\n"
           "struct " << common::openFramingHeaderStr() << " : public\n" <<
           output::indent(1) << "comms::field::Bundle<\n" <<
           output::indent(2) << BigEndianStr << ",\n" <<
           output::indent(2) << "std::tuple<\n" <<
           output::indent(3) << "comms::field::IntValue<\n" <<
           output::indent(4) << BigEndianStr << ",\n" <<
           output::indent(4) << "std::uint32_t,\n" <<
           output::indent(4) << "comms::option::NumValueSerOffset<sizeof(std::uint32_t) + sizeof(std::uint16_t)>\n" <<
           output::indent(3) << ">,\n" <<
           output::indent(3) << "comms::field::IntValue<\n" <<
           output::indent(4) << BigEndianStr << ",\n" <<
           output::indent(4) << "std::uint16_t,\n" <<
           output::indent(4) << "comms::option::ValidNumValue<" << sync << ">,\n" <<
           output::indent(4) << "comms::option::DefaultNumValue<" << sync << ">,\n" <<
           output::indent(4) << "comms::option::FailOnInvalid<comms::ErrorStatus::ProtocolError>\n" <<
           output::indent(3) << ">\n" <<
           output::indent(2) << ">\n" <<
           output::indent(1) << ">\n" <<
           "{\n" <<
           output::indent(1) << "/// \\brief Allow access to internal fields.\n" <<
           output::indent(1) << "/// \\details See definition of \\b COMMS_FIELD_MEMBERS_ACCESS macro\n" <<
           output::indent(1) << "///     related to \\b comms::field::Bundle class from COMMS library\n" <<
           output::indent(1) << "///     for details.\\n\n" <<
           output::indent(1) << "COMMS_FIELD_MEMBERS_ACCESS(\n" <<
           output::indent(2) << "messageLength,\n" <<
           output::indent(2) << "encodingType\n" <<
           output::indent(1) << ");\n"
           "};\n\n"
           "} // namespace " << common::builtinNamespaceNameStr() << "\n\n";
    return out.good();
}

bool writePad(DB& db)
{
    if (!common::createProtocolDefDir(db.getRootPath(), db.getProtocolNamespace(), common::builtinNamespaceNameStr())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(db.getProtocolNamespace(), common::builtinNamespaceNameStr() + '/' + common::padStr() + ".h");
    auto filePath = bf::path(db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "/// \\file\n"
           "/// \\brief Contains definition of implicitly defined \\ref " << common::builtinNamespaceStr() << common::padStr() << "\n"
           "\n\n"
           "#pragma once\n\n"
           "#include <cstdint>\n\n"
           "#include \"comms/Field.h\"\n"
           "#include \"comms/field/ArrayList.h\"\n"
           "#include \"comms/options.h\"\n\n"
           "namespace " << common::builtinNamespaceNameStr() << "\n"
           "{\n\n"
           "/// \\brief Padding type definition.\n"
           "/// \\tparam TFieldBase Base class of all the fields.\n"
           "/// \\tparam TLen Length of the padding.\n"
           "/// \\tparam TOpt Extra options...\n"
           "template <\n" <<
           output::indent(1) << "typename TFieldBase,\n" <<
           output::indent(1) << "std::size_t TLen,\n" <<
           output::indent(1) << "typename... TOpt\n" <<
           ">\n"
           "struct " << common::padStr() << " : public\n" <<
           output::indent(1) << "comms::field::ArrayList<\n" <<
           output::indent(2) << "TFieldBase,\n" <<
           output::indent(2) << "std::uint8_t,\n" <<
           output::indent(2) << "TOpt...,\n" <<
           output::indent(2) << "comms::option::SequenceFixedSize<TLen>\n" <<
           output::indent(1) << ">\n" <<
           "{\n";
    common::writeDefaultSetVersionFunc(out, 1);
    out << "};\n\n"
           "} // namespace " << common::builtinNamespaceNameStr() << "\n\n";
    return out.good();
}

bool writeVersionSetter(DB& db)
{
    if (!common::createProtocolDefDir(db.getRootPath(), db.getProtocolNamespace(), common::builtinNamespaceNameStr())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(db.getProtocolNamespace(), common::builtinNamespaceNameStr() + '/' + common::versionSetterFileName());
    auto filePath = bf::path(db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "/// \\file\n"
           "/// \\brief Contains definition of helper class \\ref " << common::builtinNamespaceStr() << common::versionSetterStr() << "\n"
           "\n\n"
           "#pragma once\n\n"
           "namespace " << common::builtinNamespaceNameStr() << "\n"
           "{\n\n"
           "/// \\brief Helper class to update version of the fields in tuple.\n"
           "/// \\details Expected to be used with \\b comms::util::tupleAccumulate() function.\n"
           "struct " << common::versionSetterStr() << '\n' <<
           "{\n" <<
           output::indent(1) << common::versionSetterStr() << "(unsigned version) : m_version(version) {}\n\n" <<
           output::indent(1) << "template <typename TField>\n" <<
           output::indent(1) << "bool operator()(bool soFar, TField& field)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return field.setVersion(m_version) || soFar;\n" <<
           output::indent(1) << "}\n\n"
           "private:\n" <<
           output::indent(1) << "unsigned m_version = 0U;\n"
           "};\n\n"
           "} // namespace " << common::builtinNamespaceNameStr() << "\n\n";
    return out.good();
}

} // namespace

BuiltIn::BuiltIn(DB& db)
  : m_db(db)
{
}

bool BuiltIn::write()
{
    auto builtIns = m_db.getAllUsedBuiltInTypes();
    for (auto& t : builtIns) {
        if (!writeBuiltIn(m_db, t)) {
            return false;
        }
    }

    if (m_db.isPaddingRecorded() && (!writePad(m_db))) {
        return false;
    }

    if (m_db.isGroupListRecorded() && (!writeGroupList(m_db))) {
        return false;
    }

    if (!writeOpenFrameHeader(m_db)) {
        return false;
    }

    if (!writeVersionSetter(m_db)) {
        return false;
    }
    return true;
}

} // namespace sbe2comms
