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

#include "MessageHeaderLayer.h"

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

bool MessageHeaderLayer::write()
{
    return writeProtocolDef();
}

bool MessageHeaderLayer::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace())) {
        return false;
    }

    auto relPath = common::protocolDirRelPath(m_db.getProtocolNamespace(), common::messageHeaderLayerFileName());
    auto filePath = bf::path(m_db.getRootPath()) / relPath;

    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& messageHeaderType = m_db.getMessageHeaderType();
    if (messageHeaderType.empty()) {
        log::error() << "Unknown message header type" << std::endl;
        return false;
    }

    auto* messageHeader = m_db.findType(messageHeaderType);
    if (messageHeader == nullptr) {
        assert(!"Something is wrong");
        return false;
    }

    auto& ns = m_db.getProtocolNamespace();
    std::set<std::string> headers;
    headers.insert("\"comms/util/Tuple.h\"");
    headers.insert("\"comms/protocol/ProtocolLayerBase.h\"");
    headers.insert("\"comms/MsgFactory.h\"");
    headers.insert('\"' + common::pathTo(ns, common::defaultOptionsFileName()) + '\"');
    headers.insert('\"' + common::pathTo(ns, common::fieldNamespaceNameStr() + '/' + messageHeader->getName() + ".h") + '\"');


    out << "/// \\file\n"
           "/// \\brief Contains definition of MessageHeaderLayer transport layer.\n\n"
           "#pragma once\n\n";

    common::writeExtraHeaders(out, headers);
    common::writeProtocolNamespaceBegin(ns, out);

    auto opts = messageHeader->getExtraOptInfos();
    auto& name = common::messageHeaderLayerStr();
    auto fieldName = name + common::optFieldSuffixStr();

    out << "/// \\brief Re-definition of the \\\"messageHeader\\\" field to be used in \\ref MessageHeaderLayer\n"
           "/// \\tparam TOpt Protocol definition options, expected to be \\ref DefaultOptions or\n"
           "///     deriving class.\n"
           "template <typename TOpt>\n"
           "using " << fieldName << "=\n" <<
           output::indent(1) << common::fieldNamespaceStr() << messageHeaderType << "<\n";
    for (auto& o : opts) {
        out << output::indent(2) << common::optParamPrefixStr() << common::fieldNamespaceStr() << o.second;
        bool comma = (&o != &opts.back());
        if (comma) {
            out << ',';
        }
        out << '\n';
    }
    out << output::indent(1) << ">;\n\n" <<
           "/// \\brief Protocol layer that uses \\ref " << common::fieldNamespaceStr() << messageHeaderType << " field as a prefix to all the\n"
           "///        subsequent data written by other (next) layers.\n"
           "/// \\details The main purpose of this layer is to process the message header information.\n"
           "///     Holds instance of \\b comms::MsgFactory as its private member and uses it\n"
           "///     to create message with the required ID.\n"
           "/// \\tparam TMessage Interface class for the \\b input messages, expected to be\n"
           "///     a variant of \\ref Message class.\n"
           "/// \\tparam TAllMessages Types of all \\b input messages, bundled in std::tuple,\n"
           "///     that this protocol stack must be able to \\b read() as well as create (using createMsg()).\n"
           "/// \\tparam TNextLayer Next transport layer type.\n"
           "/// \\tparam TField Field of message header.\n"
           "/// \\tparam TFactoryOpt All the options that will be forwarded to definition of\n"
           "///     message factory type (comms::MsgFactory).\n"
           "/// \\headerfile MessageHeaderLayer.h\n"
           "template <\n" <<
           output::indent(1) << "typename TMessage,\n" <<
           output::indent(1) << "typename TAllMessages,\n" <<
           output::indent(1) << "typename TNextLayer,\n" <<
           output::indent(1) << "typename TField = " << fieldName << "<DefaultOptions>,\n" <<
           output::indent(1) << "typename TFactoryOpt = comms::option::EmptyOption\n"
           ">\n"
           "class " << name << " : public\n" <<
           output::indent(1) << "comms::protocol::ProtocolLayerBase<\n" <<
           output::indent(2) << "TField,\n" <<
           output::indent(2) << "TNextLayer,\n" <<
           output::indent(2) << name << "<TMessage, TAllMessages, TNextLayer, TField, TFactoryOpt>\n" <<
           output::indent(1) << ">\n"
           "{\n" <<
           output::indent(1) << "static_assert(comms::util::IsTuple<TAllMessages>::Value,\n" <<
           output::indent(2) << "\"TAllMessages must be of std::tuple type\");\n\n" <<
           output::indent(1) << "using BaseImpl =\n" <<
           output::indent(2) << "comms::protocol::ProtocolLayerBase<\n" <<
           output::indent(3) << "TField,\n" <<
           output::indent(3) << "TNextLayer,\n" <<
           output::indent(3) << name << "<TMessage, TAllMessages, TNextLayer, TField, TFactoryOpt>\n" <<
           output::indent(2) << ">;\n\n" <<
           output::indent(1) << "using Factory = comms::MsgFactory<TMessage, TAllMessages, TFactoryOpt>;\n\n" <<
           output::indent(1) << "static_assert(TMessage::InterfaceOptions::HasMsgIdType,\n" <<
           output::indent(2) << "\"Usage of MessageHeaderLayer requires support for ID type. \"\n" <<
           output::indent(2) << "\"Use comms::option::MsgIdType option in message interface type definition.\");\n\n" <<
           "public:\n" <<
           output::indent(1) << "/// \\brief All supported message types bundled in std::tuple.\n" <<
           output::indent(1) << "/// \\see comms::MsgFactory::AllMessages.\n" <<
           output::indent(1) << "using AllMessages = typename Factory::AllMessages;\n\n" <<
           output::indent(1) << "/// \\brief Type of smart pointer that will hold allocated message object.\n" <<
           output::indent(1) << "/// \\details Same as \\b comms::MsgFactory::MsgPtr.\n" <<
           output::indent(1) << "using MsgPtr = typename Factory::MsgPtr;\n\n" <<
           output::indent(1) << "/// \\brief Type of the \\b input message interface.\n" <<
           output::indent(1) << "using Message = TMessage;\n\n" <<
           output::indent(1) << "/// \\brief Type of message ID\n" <<
           output::indent(1) << "using MsgIdType = typename Message::MsgIdType;\n\n" <<
           output::indent(1) << "/// \\brief Type of message ID when passed by the parameter\n" <<
           output::indent(1) << "using MsgIdParamType = typename Message::MsgIdParamType;\n\n" <<
           output::indent(1) << "/// \\brief Type of the field object used to read/write message ID value.\n" <<
           output::indent(1) << "using Field = typename BaseImpl::Field;\n\n" <<
           output::indent(1) << "/// \\brief Default constructor.\n" <<
           output::indent(1) << name << "() = default;\n\n" <<
           output::indent(1) << "/// \\brief Copy constructor.\n" <<
           output::indent(1) << name << "(const " << name << "&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Move constructor.\n" <<
           output::indent(1) << name << '(' << name << "&&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Copy assignment.\n" <<
           output::indent(1) << name << "& operator=(const " << name << "&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Move assignment.\n" <<
           output::indent(1) << name << "& operator=(" << name << "&&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Destructor\n" <<
           output::indent(1) << '~' << name << "() noexcept = default;\n\n" <<
           output::indent(1) << "/// \\brief Deserialise message from the input data sequence.\n" <<
           output::indent(1) << "/// \\details The function will read message header from the data sequence first,\n" <<
           output::indent(1) << "///     generate appropriate message object based on the read ID and\n" <<
           output::indent(1) << "///     forward the read() request to the next layer.\n" <<
           output::indent(1) << "///     If the message object cannot be generated (the message type is not\n" <<
           output::indent(1) << "///     provided inside \\b TAllMessages template parameter), but\n" <<
           output::indent(1) << "///     the \\b comms::option::SupportGenericMessage option has beed used,\n" <<
           output::indent(1) << "///     the \\b comms::GenericMessage may be generated instead.\n" <<
           output::indent(1) << "/// \\tparam TIter Type of iterator used for reading.\n" <<
           output::indent(1) << "/// \\tparam TNextLayerReader next layer reader object type.\n" <<
           output::indent(1) << "/// \\param[out] header Message header field object to read.\n" <<
           output::indent(1) << "/// \\param[in, out] msgPtr Reference to smart pointer that will hold\n" <<
           output::indent(1) << "///                 allocated message object\n" <<
           output::indent(1) << "/// \\param[in, out] iter Input iterator used for reading.\n" <<
           output::indent(1) << "/// \\param[in] size Size of the data in the sequence\n" <<
           output::indent(1) << "/// \\param[out] missingSize If not nullptr and return value is\n" <<
           output::indent(1) << "///             comms::ErrorStatus::NotEnoughData it will contain\n" <<
           output::indent(1) << "///             minimal missing data length required for the successful\n" <<
           output::indent(1) << "///             read attempt.\n" <<
           output::indent(1) << "/// \\param[in] nextLayerReader Next layer reader object.\n" <<
           output::indent(1) << "/// \\return Status of the operation.\n" <<
           output::indent(1) << "/// \\pre msgPtr doesn't point to any object:\n" <<
           output::indent(1) << "///      \\code assert(!msgPtr); \\endcode\n" <<
           output::indent(1) << "/// \\pre Iterator must be valid and can be dereferenced and incremented at\n" <<
           output::indent(1) << "///      least \"size\" times;\n" <<
           output::indent(1) << "/// \\post The iterator will be advanced by the number of bytes was actually\n" <<
           output::indent(1) << "///       read. In case of an error, distance between original position and\n" <<
           output::indent(1) << "///       advanced will pinpoint the location of the error.\n" <<
           output::indent(1) << "/// \\post Returns comms::ErrorStatus::Success if and only if msgPtr points\n" <<
           output::indent(1) << "///       to a valid object.\n" <<
           output::indent(1) << "/// \\post missingSize output value is updated if and only if function\n" <<
           output::indent(1) << "///       returns comms::ErrorStatus::NotEnoughData.\n" <<
           output::indent(1) << "template <typename TIter, typename TNextLayerReader>\n" <<
           output::indent(1) << "comms::ErrorStatus doRead(\n" <<
           output::indent(2) << "Field& header,\n" <<
           output::indent(2) << "MsgPtr& msgPtr,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "std::size_t* missingSize,\n" <<
           output::indent(2) << "TNextLayerReader&& nextLayerReader)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "auto es = header.read(iter, size);\n" <<
           output::indent(2) << "if (es == comms::ErrorStatus::NotEnoughData) {\n" <<
           output::indent(3) << "BaseImpl::updateMissingSize(header, size, missingSize);\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "auto id = header.field_templateId().value();\n" <<
           output::indent(2) << "do {\n" <<
           output::indent(3) << "msgPtr = createMsg(id);\n" <<
           output::indent(3) << "if (msgPtr) {\n" <<
           output::indent(4) << "break;\n" <<
           output::indent(3) << "}\n\n" <<
           output::indent(3) << "msgPtr = factory_.createGenericMsg(id);\n" <<
           output::indent(3) << "if (msgPtr) {\n" <<
           output::indent(4) << "break;\n" <<
           output::indent(3) << "}\n\n" <<
           output::indent(3) << "return comms::ErrorStatus::InvalidMsgId;\n" <<
           output::indent(2) << "} while (false);\n\n" <<
           output::indent(2) << "msgPtr->setBlockLength(header.field_blockLength().value());\n" <<
           output::indent(2) << "msgPtr->setVersion(header.field_version().value());\n" <<
           output::indent(2) << "es = nextLayerReader.read(msgPtr, iter, size - header.length(), missingSize);\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "msgPtr.reset();\n" <<
           output::indent(2) << "}\n" <<
           output::indent(2) << "return es;\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Serialise message into output data sequence.\n" <<
           output::indent(1) << "/// \\details The function will write \\ref " << common::fieldNamespaceStr() << messageHeaderType << " to the data\n" <<
           output::indent(1) << "///     sequence, then call write() member function of the next\n" <<
           output::indent(1) << "///     protocol layer. If \\b TMsg type is recognised to be actual message\n" <<
           output::indent(1) << "///     type (inherited from comms::MessageBase while using\n" <<
           output::indent(1) << "///     comms::option::StaticNumIdImpl option to specify its numeric ID),\n" <<
           output::indent(1) << "///     its defined \\b doGetId() member function (see \\b comms::MessageBase::doGetId())\n" <<
           output::indent(1) << "///     non virtual function is called. Otherwise polymorphic \\b getId()\n" <<
           output::indent(1) << "///     member function is used to retrieve the message ID information, which\n" <<
           output::indent(1) << "///     means the message interface class must use \\b comms::option::IdInfoInterface\n" <<
           output::indent(1) << "///     option to define appropriate interface.\n" <<
           output::indent(1) << "/// \\tparam TMsg Type of the message being written.\n" <<
           output::indent(1) << "/// \\tparam TIter Type of iterator used for writing.\n" <<
           output::indent(1) << "/// \\tparam TNextLayerWriter next layer writer object type.\n" <<
           output::indent(1) << "/// \\param[out] header Message header field object to update and write.\n" <<
           output::indent(1) << "/// \\param[in] msg Reference to message object\n" <<
           output::indent(1) << "/// \\param[in, out] iter Output iterator used for writing.\n" <<
           output::indent(1) << "/// \\param[in] size Max number of bytes that can be written.\n" <<
           output::indent(1) << "/// \\param[in] nextLayerWriter Next layer writer object.\n" <<
           output::indent(1) << "/// \\return Status of the write operation.\n" <<
           output::indent(1) << "/// \\pre Iterator must be valid and can be dereferenced and incremented at\n" <<
           output::indent(1) << "///      least \"size\" times;\n" <<
           output::indent(1) << "/// \\post The iterator will be advanced by the number of bytes was actually\n" <<
           output::indent(1) << "///       written. In case of an error, distance between original position\n" <<
           output::indent(1) << "///       and advanced will pinpoint the location of the error.\n" <<
           output::indent(1) << "/// \\return Status of the write operation.\n" <<
           output::indent(1) << "template <typename TMsg, typename TIter, typename TNextLayerWriter>\n" <<
           output::indent(1) << "comms::ErrorStatus doWrite(\n" <<
           output::indent(2) << "Field& header,\n" <<
           output::indent(2) << "const TMsg& msg,\n" <<
           output::indent(2) << "TIter& iter,\n" <<
           output::indent(2) << "std::size_t size,\n" <<
           output::indent(2) << "TNextLayerWriter&& nextLayerWriter) const\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "using MsgType = typename std::decay<decltype(msg)>::type;\n\n" <<
           output::indent(2) << "auto blockLength = \n" <<
           output::indent(3) << "static_cast<typename " << common::fieldNamespaceStr() << messageHeaderType << common::memembersSuffixStr() << "::blockLength<>::ValueType>(\n" <<
           output::indent(4) << "msg.getBlockLength());\n" <<
           output::indent(2) << "auto version = \n" <<
           output::indent(3) << "static_cast<typename " << common::fieldNamespaceStr() << messageHeaderType << common::memembersSuffixStr() << "::version<>::ValueType>(\n" <<
           output::indent(4) << "msg.getVersion());\n\n" <<
           output::indent(2) << "header.field_blockLength().value() = blockLength;\n" <<
           output::indent(2) << "header.field_templateId().value() = getMsgId(msg, IdRetrieveTag<MsgType>());\n" <<
           output::indent(2) << "header.field_version().value() = version;\n\n" <<
           output::indent(2) << "auto es = header.write(iter, size);\n" <<
           output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
           output::indent(3) << "return es;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "return nextLayerWriter.write(msg, iter, size - header.length());\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "/// \\brief Create message object given message id\n" <<
           output::indent(1) << "/// \\details Hides and overrides createMsg() function inherited from\n" <<
           output::indent(1) << "///     \\b comms::protocol::ProtocolLayerBase. This function forwards the request to the\n" <<
           output::indent(1) << "///     message factory object (\\b comms::MsgFactory) embedded as a private\n" <<
           output::indent(1) << "///     data member of this class.\n" <<
           output::indent(1) << "/// \\param[in] id ID of the message\n" <<
           output::indent(1) << "/// \\param[in] idx Relative index of the message with the same ID.\n" <<
           output::indent(1) << "/// \\return Smart pointer to the created message object.\n" <<
           output::indent(1) << "/// \\see comms::MsgFactory::createMsg()\n" <<
           output::indent(1) << "MsgPtr createMsg(MsgIdParamType id, unsigned idx = 0)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return factory_.createMsg(id, idx);\n" <<
           output::indent(1) << "}\n\n" <<
           "private:\n" <<
           output::indent(1) << "struct PolymorphicIdTag {};\n" <<
           output::indent(1) << "struct DirectIdTag {};\n\n" <<
           output::indent(1) << "template <typename TMsg>\n" <<
           output::indent(1) << "using IdRetrieveTag =\n" <<
           output::indent(2) << "typename std::conditional<\n" <<
           output::indent(3) << "comms::protocol::details::ProtocolLayerHasDoGetId<TMsg>::Value,\n" <<
           output::indent(3) << "DirectIdTag,\n" <<
           output::indent(3) << "PolymorphicIdTag\n" <<
           output::indent(2) << ">::type;\n\n" <<
           output::indent(1) << "template <typename TMsg>\n" <<
           output::indent(1) << "static MsgIdParamType getMsgId(const TMsg& msg, PolymorphicIdTag)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "using MsgType = typename std::decay<decltype(msg)>::type;\n" <<
           output::indent(2) << "static_assert(comms::protocol::details::ProtocolLayerHasInterfaceOptions<MsgType>::Value,\n" <<
           output::indent(3) << "\"The message class is expected to inherit from comms::Message\");\n" <<
           output::indent(2) << "static_assert(MsgType::InterfaceOptions::HasMsgIdInfo,\n" <<
           output::indent(3) << "\"The message interface class must expose polymorphic ID retrieval functionality, \"\n" <<
           output::indent(3) << "\"use comms::option::IdInfoInterface option to define it.\");\n\n" <<
           output::indent(3) << "return msg.getId();\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(1) << "template <typename TMsg>\n" <<
           output::indent(1) << "static constexpr MsgIdParamType getMsgId(const TMsg& msg, DirectIdTag)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return msg.doGetId();\n" <<
           output::indent(1) << "}\n\n" <<
           output::indent(1) << "Factory factory_;\n" <<
           "};\n\n";

    common::writeProtocolNamespaceEnd(ns, out);
    return true;
}

} // namespace sbe2comms
