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

#include "OpenFramingHeaderLayer.h"

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

bool OpenFramingHeaderLayer::write()
{
    return writeProtocolDef();
}

bool OpenFramingHeaderLayer::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(m_db.getProtocolNamespace(), common::openFramingHeaderLayerFileName());
    auto filePath = bf::path(m_db.getRootPath()) / relPath;

    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& ns = m_db.getProtocolNamespace();

    std::string fieldInc('\"' + common::builtinNamespaceNameStr() + '/' + common::openFramingHeaderStr() + ".h\"");
    std::string fieldType(common::builtinNamespaceStr() + common::openFramingHeaderStr());

    bool hasOpenFrameHeaderDefined = m_db.hasSimpleOpenFramingHeaderTypeDefined();
    if (hasOpenFrameHeaderDefined) {
        auto& fieldName = m_db.getSimpleOpenFramingHeaderTypeName();
        fieldType = common::fieldNamespaceStr() + fieldName;
        fieldInc = common::localHeader(ns, common::fieldNamespaceNameStr(), fieldName + ".h");
    }

    out << "/// \\file\n"
           "/// \\brief Contains definition of " << common::scopeFor(ns, common::openFramingHeaderLayerStr()) << " transport layer.\n\n"
           "#pragma once\n\n"
           "#include <iterator>\n"
           "#include <type_traits>\n\n"
           "#include \"comms/protocol/ProtocolLayerBase.h\"\n"
           "#include " << common::localHeader(ns, common::emptyString(), common::defaultOptionsFileName()) << "\n" <<
           "#include " << fieldInc << "\n\n";

    common::writeProtocolNamespaceBegin(ns, out);

    auto redefName = common::openFramingHeaderLayerStr() + common::optFieldSuffixStr();

    out << "/// \\brief Re-definition of the Simple Open Framing Header field to be used in \\ref " << common::openFramingHeaderLayerStr() << "\n"
           "/// \\tparam TOpt Protocol definition options, expected to be \\ref " << common::defaultOptionsStr() << " or\n"
           "///     deriving class.\n"
           "template <typename TOpt>\n"
           "using " << redefName << " = ";
    if (hasOpenFrameHeaderDefined) {
        auto* headerType = m_db.findType(m_db.getSimpleOpenFramingHeaderTypeName());
        assert(headerType != nullptr);
        auto opts = headerType->getExtraOptInfos();
        out << '\n' <<
               output::indent(1) << fieldType << "<\n";
        for (auto& o : opts) {
            out << output::indent(2) << common::optParamPrefixStr() << common::fieldNamespaceStr() << o.second;
            bool comma = (&o != &opts.back());
            if (comma) {
                out << ',';
            }
            out << '\n';
        }
        out << output::indent(1) << ">;\n\n";
    }
    else {
        out << common::builtinNamespaceStr() << common::openFramingHeaderStr() << ";\n\n";
    }

    auto& name = common::openFramingHeaderLayerStr();

    out << "/// \\brief Protocol layer that uses \\ref " << redefName << " field as a prefix to all the\n"
           "///        subsequent data written by other (next) layers.\n"
           "/// \\details The main purpose of this layer is to provide information about\n" <<
           "///     the remaining size of the serialised message. Inherits from \\b comms::protocol::ProtocolLayerBase.\n"
           "///     Please read the documentation of the latter for details on inherited public\n"
           "///     interface. Please also read <b>Protocol Stack Tutorial</b> page from the \\b COMMS\n"
           "///     library documentation.\n"
           "/// \\tparam TNextLayer Next transport layer in protocol stack.\n"
           "/// \\tparam TField Field of the Simple Open Framing Header.\n";;

    auto writeBaseDefFunc =
        [&out, &name](unsigned indent)
        {
            out << output::indent(indent) << "comms::protocol::ProtocolLayerBase<\n" <<
                   output::indent(indent + 1) << "TField" << ",\n" <<
                   output::indent(indent + 1) << "TNextLayer,\n" <<
                   output::indent(indent + 1) << name << "<TNextLayer, TField>\n" <<
                   output::indent(indent) << ">";
        };

    out << "template <\n" <<
           output::indent(1) << "typename TNextLayer,\n" <<
           output::indent(1) << "typename TField = " << redefName << "<DefaultOptions>\n"
           ">\n"
           "class " << name << " : public\n";
    writeBaseDefFunc(1);
    out << "\n{\n" <<
           output::indent(1) << "using BaseImpl =\n";
    writeBaseDefFunc(2);
    out << ";\n\n"
           "public:\n" <<
           output::indent(1) << "/// \\brief Type of the field object used to read/write header.\n" <<
           output::indent(1) << "using Field = typename BaseImpl::Field;\n\n" <<
           output::indent(1) << "/// \\brief Default constructor\n" <<
           output::indent(1) << name << "() = default;\n\n" <<
           output::indent(1) << "/// \\brief Copy constructor\n" <<
           output::indent(1) << name << "(const " << name << "&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Move constructor\n" <<
           output::indent(1) << name << "(" << name << "&&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Destructor.\n" <<
           output::indent(1) << '~' << name << "() noexcept = default;\n\n" <<
           output::indent(1) << "/// \\brief Copy assignment.\n" <<
           output::indent(1) << name << "& operator=(const " << name << "&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Move assignment.\n" <<
           output::indent(1) << name << "& operator=(" << name << "&&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Deserialise message from the input data sequence.\n" <<
           output::indent(1) << "/// \\details Reads the header data from the input data sequence\n" <<
           output::indent(1) << "///          and calls read() member function of the next layer with\n" <<
           output::indent(1) << "///          the size specified in the \"messageLength\" field.The function will also\n" <<
           output::indent(1) << "///          compare the provided size of the data with size of the\n" <<
           output::indent(1) << "///          message read from the buffer. If the latter is greater than\n" <<
           output::indent(1) << "///          former, \\b comms::ErrorStatus::NotEnoughData will be returned.\n" <<
           output::indent(1) << "///          However, if buffer contains enough data, but the next layer\n" <<
           output::indent(1) << "///          reports it's not enough (returns comms::ErrorStatus::NotEnoughData),\n" <<
           output::indent(1) << "///          \\b comms::ErrorStatus::ProtocolError will be returned.\n" <<
           output::indent(1) << "/// \\tparam TMsgPtr Type of smart pointer that holds message object.\n" <<
           output::indent(1) << "/// \\tparam TIter Type of iterator used for reading.\n" <<
           output::indent(1) << "/// \\tparam TNextLayerReader next layer reader object type.\n" <<
           output::indent(1) << "/// \\param[out] field Field object to read.\n" <<
           output::indent(1) << "/// \\param[in, out] msgPtr Reference to smart pointer that already holds or\n" <<
           output::indent(1) << "///     will hold allocated message object.\n" <<
           output::indent(1) << "/// \\param[in, out] iter Input iterator used for reading.\n" <<
           output::indent(1) << "/// \\param[in] size Size of the data in the sequence\n" <<
           output::indent(1) << "/// \\param[out] missingSize If not nullptr and return value is\n" <<
           output::indent(1) << "///     comms::ErrorStatus::NotEnoughData it will contain\n" <<
           output::indent(1) << "///     minimal missing data length required for the successful\n" <<
           output::indent(1) << "///     read attempt.\n" <<
           output::indent(1) << "/// \\param[in] nextLayerReader Next layer reader object.\n" <<
           output::indent(1) << "/// \\return Status of the read operation.\n" <<
           output::indent(1) << "/// \\pre Iterator must be valid and can be dereferenced and incremented at\n" <<
           output::indent(1) << "///      least \"size\" times.\n" <<
           output::indent(1) << "/// \\post The iterator will be advanced by the number of bytes was actually\n" <<
           output::indent(1) << "///       read. In case of an error, distance between original position and\n" <<
           output::indent(1) << "///       advanced will pinpoint the location of the error.\n" <<
           output::indent(1) << "/// \\post missingSize output value is updated if and only if function\n" <<
           output::indent(1) << "///       returns comms::ErrorStatus::NotEnoughData.\n" <<
           output::indent(1) << "template <typename TMsgPtr, typename TIter, typename TNextLayerReader>\n" <<
           output::indent(1) << "comms::ErrorStatus doRead(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "TMsgPtr& msgPtr,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "std::size_t* missingSize,\n" <<
           output::indent(2) << "TNextLayerReader&& nextLayerReader)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "using IterType = typename std::decay<decltype(iter)>::type;\n" <<
           output::indent(2) << "using IterTag = typename std::iterator_traits<IterType>::iterator_category;\n" <<
           output::indent(2) << "static_assert(\n" <<
           output::indent(3) << "std::is_base_of<std::random_access_iterator_tag, IterTag>::value,\n" <<
           output::indent(3) << "\"Current implementation of " << name << " requires iterator\"\n" <<
           output::indent(3) << "\"used for reading to be random-access one.\");\n\n" <<
           output::indent(2) << "auto es = field.read(iter, size);\n" <<
           output::indent(2) << "if (es == comms::ErrorStatus::NotEnoughData) {\n" <<
           output::indent(3) << "BaseImpl::updateMissingSize(field, size, missingSize);\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "auto fromIter = iter;\n" <<
           output::indent(2) << "auto actualRemainingSize = (size - field.length());\n" <<
           output::indent(2) << "auto requiredRemainingSize = static_cast<std::size_t>(field.field_messageLength().value());\n\n" <<
           output::indent(2) << "if (actualRemainingSize < requiredRemainingSize) {\n" <<
           output::indent(3) << "if (missingSize != nullptr) {\n" <<
           output::indent(4) << "*missingSize = requiredRemainingSize - actualRemainingSize;\n" <<
           output::indent(3) << "}\n" <<
           output::indent(3) << "return comms::ErrorStatus::NotEnoughData;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "// not passing missingSize farther on purpose\n" <<
           output::indent(2) << "es = nextLayerReader.read(msgPtr, iter, requiredRemainingSize, nullptr);\n" <<
           output::indent(2) << "if (es == comms::ErrorStatus::NotEnoughData) {\n" <<
           output::indent(3) << "return comms::ErrorStatus::ProtocolError;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "auto consumed =\n" <<
           output::indent(3) << "static_cast<std::size_t>(std::distance(fromIter, iter));\n" <<
           output::indent(2) << "if (consumed < requiredRemainingSize) {\n" <<
           output::indent(3) << "auto diff = requiredRemainingSize - consumed;\n" <<
           output::indent(3) << "std::advance(iter, diff);\n" <<
           output::indent(2) << "}\n" <<
           output::indent(2) << "return es;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Serialise message into the output data sequence.\n" <<
           output::indent(1) << "/// \\details The function will write header data,\n" <<
           output::indent(1) << "///     then invoke the write() member function of the next\n" <<
           output::indent(1) << "///     layer. The calculation of the required message length is performed by invoking\n" <<
           output::indent(1) << "///     \"length(msg)\".\n" <<
           output::indent(1) << "/// \\tparam TMsg Type of message object.\n" <<
           output::indent(1) << "/// \\tparam TIter Type of iterator used for writing.\n" <<
           output::indent(1) << "/// \\tparam TNextLayerWriter next layer writer object type.\n" <<
           output::indent(1) << "/// \\param[out] field Field object to update and write.\n" <<
           output::indent(1) << "/// \\param[in] msg Reference to message object\n" <<
           output::indent(1) << "/// \\param[in, out] iter Output iterator.\n" <<
           output::indent(1) << "/// \\param[in] size Max number of bytes that can be written.\n" <<
           output::indent(1) << "/// \\param[in] nextLayerWriter Next layer writer object.\n" <<
           output::indent(1) << "/// \\return Status of the write operation.\n" <<
           output::indent(1) << "/// \\pre Iterator must be valid and can be dereferenced and incremented at\n" <<
           output::indent(1) << "///      least \"size\" times.\n" <<
           output::indent(1) << "/// \\post The iterator will be advanced by the number of bytes was actually\n" <<
           output::indent(1) << "///       written. In case of an error, distance between original position\n" <<
           output::indent(1) << "///       and advanced will pinpoint the location of the error.\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TNextLayerWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus doWrite(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TNextLayerWriter&& nextLayerWriter) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "using MsgType = typename std::decay<decltype(msg)>::type;\n" <<
           output::indent(2) << "return\n" <<
           output::indent(3) << "writeInternal(\n" <<
           output::indent(4) << "field,\n" <<
           output::indent(4) << "msg,\n" <<
           output::indent(4) << "iter,\n" <<
           output::indent(4) << "size,\n" <<
           output::indent(4) << "std::forward<TNextLayerWriter>(nextLayerWriter),\n" <<
           output::indent(4) << "MsgLengthTag<MsgType>());\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Update written dummy size with proper value.\n" <<
           output::indent(1) << "/// \\details Should be called when \\ref doWrite() returns \\b comms::ErrorStatus::UpdateRequired.\n" <<
           output::indent(1) << "/// \\tparam TIter Type of iterator used for updating.\n" <<
           output::indent(1) << "/// \\tparam TNextLayerWriter next layer updater object type.\n" <<
           output::indent(1) << "/// \\param[out] field Field object to update.\n" <<
           output::indent(1) << "/// \\param[in, out] iter Any random access iterator.\n" <<
           output::indent(1) << "/// \\param[in] size Number of bytes that have been written using write().\n" <<
           output::indent(1) << "/// \\param[in] nextLayerUpdater Next layer updater object.\n" <<
           output::indent(1) << "/// \\return Status of the update operation.\n" <<
           output::indent(1) << "template <typename TIter, typename TNextLayerUpdater>\n" <<
           output::indent(1) << "comms::ErrorStatus doUpdate(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TNextLayerUpdater&& nextLayerUpdater) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "field.field_messageLength().value() = size - Field::maxLength();\n\n" <<
           output::indent(2) << "auto es = field.write(iter, size);\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "return nextLayerUpdater.update(iter, size - field.length());\n" <<
           output::indent(1) << "}\n\n"
           "private:\n\n" <<
           output::indent(1) << "struct MsgHasLengthTag {};\n" <<
           output::indent(1) << "struct MsgNoLengthTag {};\n\n" <<
           output::indent(1) << "template<typename TMsg>\n" <<
           output::indent(1) << "using MsgLengthTag =\n" <<
           output::indent(2) << "typename std::conditional<\n" <<
           output::indent(3) << "comms::protocol::details::ProtocolLayerHasFieldsImpl<TMsg>::Value || TMsg::InterfaceOptions::HasLength,\n" <<
           output::indent(3) << "MsgHasLengthTag,\n" <<
           output::indent(3) << "MsgNoLengthTag\n" <<
           output::indent(2) << ">::type;\n\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternalHasLength(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "auto& messageLengthValue = field.field_messageLength().value();\n" <<
           output::indent(2) << "using MessageLengthValueType = typename std::decay<decltype(messageLengthValue)>::type;\n" <<
           output::indent(2) << "messageLengthValue = \n" <<
           output::indent(3) << "static_cast<MessageLengthValueType>(BaseImpl::nextLayer().length(msg));\n" <<
           output::indent(2) << "auto es = field.write(iter, size);\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "return nextLayerWriter.write(msg, iter, size - field.length());\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternalRandomAccess(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "auto valueIter = iter;\n\n" <<
           output::indent(2) << "auto& messageLengthValue = field.field_messageLength().value();\n" <<
           output::indent(2) << "messageLengthValue = 0U;\n" <<
           output::indent(2) << "auto es = field.write(iter, size);\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "auto dataIter = iter;\n" <<
           output::indent(2) << "es = nextLayerWriter.write(msg, iter, size - field.length());\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "using MessageLengthValueType = typename std::decay<decltype(messageLengthValue)>::type;\n" <<
           output::indent(2) << "messageLengthValue = static_cast<MessageLengthValueType>(std::distance(dataIter, iter));\n" <<
           output::indent(2) << "return field.write(valueIter, Field::minLength());\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternalOutput(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "field.field_messageLength().value() = 0;\n" <<
           output::indent(2) << "auto es = field.write(iter, size);\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "es = nextLayerWriter.write(msg, iter, size - field.length());\n" <<
           output::indent(2) << "if ((es != comms::ErrorStatus::Success) &&\n" <<
           output::indent(2) << "    (es != comms::ErrorStatus::UpdateRequired)) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "return comms::ErrorStatus::UpdateRequired;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternalNoLengthTagged(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter,\n" <<
           output::indent(2) << "std::random_access_iterator_tag) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return writeInternalRandomAccess(field, msg, iter, size, std::forward<TWriter>(nextLayerWriter));\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternalNoLengthTagged(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter,\n" <<
           output::indent(2) << "std::output_iterator_tag) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return writeInternalOutput(field, msg, iter, size, std::forward<TWriter>(nextLayerWriter));\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternalNoLength(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "using IterType = typename std::decay<decltype(iter)>::type;\n" <<
           output::indent(2) << "using Tag = typename std::iterator_traits<IterType>::iterator_category;\n" <<
           output::indent(2) << "return writeInternalNoLengthTagged(field, msg, iter, size, std::forward<TWriter>(nextLayerWriter), Tag());\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternal(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter,\n" <<
           output::indent(2) << "MsgHasLengthTag) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return writeInternalHasLength(field, msg, iter, size, std::forward<TWriter>(nextLayerWriter));\n" <<
           output::indent(1) << "}\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus writeInternal(\n" <<
           output::indent(2) << "Field& field,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TWriter&& nextLayerWriter,\n" <<
           output::indent(2) << "MsgNoLengthTag) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return writeInternalNoLength(field, msg, iter, size, std::forward<TWriter>(nextLayerWriter));\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "static_assert(Field::minLength() == Field::maxLength(),\n" <<
           output::indent(2) << '\"' << fieldType << " field is expected to have fixed length.\");\n\n" <<
           "};\n\n";


    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
