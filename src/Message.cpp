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

#include "common.h"
#include "prop.h"
#include "output.h"
#include "DB.h"
#include "log.h"
#include "BasicField.h"

namespace bf = boost::filesystem;

namespace sbe2comms
{

namespace
{

std::string napespacePrefix(DB& db)
{
    auto& ns = db.getProtocolNamespace();
    if (ns.empty()) {
        return ns;
    }

    return ns + '/';
}

void writeFileHeader(std::ostream& out, const std::string& name, DB& db)
{
    auto& ns = db.getProtocolNamespace();
    out << "/// \\file\n"
           "/// \\brief Contains definition of " << common::scopeFor(ns, common::messageNamespaceStr() + name) << " message and its fields.\n\n"
           "#pragma once\n\n";
}

void openNamespaces(std::ostream& out, DB& db)
{
    common::writeProtocolNamespaceBegin(db.getProtocolNamespace(), out);

    out << "namespace " << common::messageDirName() << "\n"
            "{\n"
            "\n";
}

void closeNamespaces(std::ostream& out, DB& db)
{
    out << "} // namespace " << common::messageDirName() << "\n"
            "\n";

    common::writeProtocolNamespaceEnd(db.getProtocolNamespace(), out);
}

void openPluginNamespaces(std::ostream& out, DB& db)
{
    common::writePluginNamespaceBegin(db.getProtocolNamespace(), out);

    out << "namespace " << common::messageDirName() << "\n"
            "{\n"
            "\n";
}

void closePluginNamespaces(std::ostream& out, DB& db)
{
    out << "} // namespace " << common::messageDirName() << "\n"
            "\n";

    common::writePluginNamespaceEnd(db.getProtocolNamespace(), out);
}


void openFieldsDef(std::ostream& out, const std::string& ns, const std::string& name)
{
    out <<
        "/// \\brief Accumulates details of all the " << name << " message fields.\n"
        "/// \\tparam TOpt Extra options to be passed to all fields.\n"
        "/// \\see \\ref " << name << "\n"
        "/// \\headerfile " << common::localHeader(ns, common::messageNamespaceNameStr(), name + ".h") << "\n"
        "template <typename TOpt = " << common::defaultOptionsStr() << ">\n" <<
        "struct " << name << common::fieldsSuffixStr() << "\n"
        "{\n";
}

void closeFieldsDef(std::ostream& out, const std::string& name)
{
    out << "}; // " << name << common::fieldsSuffixStr() << "\n\n";
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

    if (getId() == 0U) {
        log::error() << "Message \"" << getName() << "\" has unspecified or invalid ID." << std::endl;
        return false;
    }

    if (!createFields()) {
        return false;
    }

    return true;
}
bool Message::write()
{
    return writeProtocolDef() && writePluginHeader() && writePluginSrc();
}


bool Message::writeDefaultOptions(std::ostream& out, unsigned indent, const std::string& scope)
{
    if (m_fields.empty()) {
        return true;
    }

    auto scopeUpd = getName() + common::fieldsSuffixStr();
    auto fieldScope = scope + scopeUpd + "::";
    out << output::indent(indent) << "/// \\brief Scope for all the options for the fields of\n" <<
           output::indent(indent) << "///     \\ref " << scope << getName() << " message.\n" <<
           output::indent(indent) << "struct " << scopeUpd << '\n' <<
           output::indent(indent) << "{\n";
    bool result = true;
    for (auto& f : m_fields) {
        result = f->writeDefaultOptions(out, indent + 1, fieldScope) && result;
    }
    out << output::indent(indent) << "}; // " << scopeUpd << "\n\n";
    return result;
}

const std::string& Message::getName() const
{
    assert(!m_props.empty());
    return prop::name(m_props);
}

const std::string& Message::getReferenceName() const
{
    return common::renameKeyword(getName());
}

unsigned Message::getId() const
{
    assert(!m_props.empty());
    return prop::id(m_props);
}

bool Message::hasFields() const
{
    return !m_fields.empty();
}

bool Message::createFields()
{
    assert(m_fields.empty());
    auto children = xmlChildren(m_node);
    unsigned expOffset = 0U;
    unsigned padCount = 0;
    bool rootBlock = true;
    bool dataMembers = false;
    auto blockLength = prop::blockLength(m_props);
    auto scope = getName() + common::fieldsSuffixStr() + "::";
    unsigned lastSinceVersion = 0U;
    Field::Kind lastKind = Field::Kind::Basic;
    std::set<std::string> fieldNames;

    auto addPaddingFunc =
        [this, &padCount, &expOffset, &scope, &lastSinceVersion](xmlNodePtr c, unsigned padLen, bool before = true) -> bool
        {
            ++padCount;
            auto* padType = m_db.getPaddingType(padLen);
            if (padType == nullptr) {
                log::error() << "Failed to generate padding type for \"" << getName() << "\" message." << std::endl;
                return false;
            }

            auto padNode = xmlCreatePaddingField(padCount, padType->getName(), lastSinceVersion);
            assert(padNode);
            auto padField = Field::create(m_db, padNode.get(), scope);
            assert(padField);
            assert(padField->getKind() == Field::Kind::Basic);
            auto* castedPadMem = static_cast<BasicField*>(padField.get());
            castedPadMem->setGeneratedPadding();

            auto* padNodeName = reinterpret_cast<const char*>(padNode->name);
            if (!padField->parse()) {
                log::error() << "Failed to parse \"" << padNodeName  << "\" field of \"" << getName() << "\" message." << std::endl;
                return false;
            }

            assert(castedPadMem->getSerializationLength() == padLen);
            expOffset += padLen;
            m_fields.push_back(std::move(padField));
            if (before) {
                assert(c != nullptr);
                xmlAddPrevSibling(c, padNode.release());
            }
            else if (c != nullptr) {
                xmlAddNextSibling(c, padNode.release());
            }
            else {
                xmlAddChild(m_node, padNode.release());
            }
            return true;
        };

    for (auto c : children) {
        auto fieldPtr = Field::create(m_db, c, scope);
        if (!fieldPtr) {
            log::error() << "Unknown field kind \"" << c->name << "\"!" << std::endl;
            return false;
        }

        std::string cName(reinterpret_cast<const char*>(c->name));
        if (!fieldPtr->parse()) {
            log::error() << "Failed to parse \"" << cName  << "\" field of \"" << getName() << "\" message." << std::endl;
            return false;
        }

        if (fieldNames.find(fieldPtr->getName()) != fieldNames.end()) {
            log::error() << "Multiple fields with the same name \"" << fieldPtr->getName() << "\"" << std::endl;
            return false;
        }

        if (!fieldPtr->doesExist()) {
            continue;
        }

        auto thisKind = fieldPtr->getKind();
        if ((!rootBlock) && (thisKind == Field::Kind::Basic)) {
            log::error() << "Basic field \"" << fieldPtr->getName() << "\" of \"" << getName() << "\" message cannot follow group or data" << std::endl;
            return false;
        }

        if ((dataMembers) && (thisKind != Field::Kind::Data)) {
            log::error() << "Field \"" << fieldPtr->getName() << "\" of \"" << getName() << "\" message cannot follow other group or data" << std::endl;
            return false;
        }

        auto sinceVersion = fieldPtr->getSinceVersion();
        if ((sinceVersion < lastSinceVersion) &&
            ((lastKind == thisKind) || (lastKind != Field::Kind::Basic))) {
            log::error() << "Unexpected \"sinceVersion\" attribute value of \"" << fieldPtr->getName() << "\", expected to be greater or equal to " << lastSinceVersion << std::endl;
            return false;
        }
        lastSinceVersion = sinceVersion;
        lastKind = thisKind;

        if (thisKind == Field::Kind::Data) {
            dataMembers = true;
        }

        do {
            if (!rootBlock) {
                break;
            }

            auto offset = fieldPtr->getOffset();
            if (thisKind != Field::Kind::Basic) {
                rootBlock = false;
                offset = std::max(offset, blockLength);
            }

            if ((blockLength != 0) && (blockLength < offset)) {
                log::error() << "Invalid offset of \"" << fieldPtr->getName() << "\" or blockLength is to small." << std::endl;
                return false;
            }


            if ((offset == 0U) || (offset == expOffset)) {
                break;
            }

            if (offset < expOffset) {
                log::error() << "Invalid offset of \"" << fieldPtr->getName() <<
                                "\" field of \"" << getName() << "\" message, causing overlap.\n" << std::endl;
                return false;
            }

            auto padLen = offset - expOffset;
            if (!addPaddingFunc(c, padLen)) {
                return false;
            }

        } while (false);

        if (rootBlock) {
            assert(thisKind == Field::Kind::Basic);
            expOffset += static_cast<const BasicField*>(fieldPtr.get())->getSerializationLength();
        }

        fieldNames.insert(fieldPtr->getName());
        m_fields.push_back(std::move(fieldPtr));
    }

    if (rootBlock && (blockLength != 0) && (expOffset < blockLength)) {
        xmlNodePtr c = nullptr;
        if (!m_fields.empty()) {
            c = m_fields.back()->getNode();
        }
        return addPaddingFunc(c, blockLength - expOffset);
    }
    return true;
}

bool Message::writeFields(std::ostream& out)
{
    if (m_fields.empty()) {
        return true;
    }
    auto& msgName = getName();
    openFieldsDef(out, m_db.getProtocolNamespace(), msgName);
    bool result = true;
    for (auto& f : m_fields) {
        result = f->write(out, 1) && result;
    }

    result = writeAllFieldsDef(out) && result;
    closeFieldsDef(out, msgName);
    return result;
}

bool Message::writeAllFieldsDef(std::ostream& out)
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

        out << output::indent(2) << f->getName();
    }
    out << '\n' << output::indent(1) << ">;\n\n";
    return true;
}

bool Message::writeMessageClass(std::ostream& out)
{
    auto& n = getName();
    out <<
        "/// \\brief Definition of " << n << " message\n"
        "/// \\details ";
    auto& desc = prop::description(m_props);
    if (!desc.empty()) {
        out << desc << "\\n\n///     ";
    }
    out <<
        "Inherits from \\b comms::MessageBase\n"
        "///     while providing \\b TMsgBase as common interface class as well as\n"
        "///     various implementation options.\n";
    if (!m_fields.empty()) {
        out << "///     \\n See \\ref " << n << common::fieldsSuffixStr() << " for definition of the fields this message contains\n"
               "///         and COMMS_MSG_FIELDS_ACCESS() for fields access details.\n"
               "/// \\tparam TMsgBase Common interface class for all the messages.\n"
               "/// \\tparam TOpt Extra options to be passed to all fields.\n";
    }
    out << "/// \\headerfile " << common::localHeader(m_db.getProtocolNamespace(), common::messageNamespaceNameStr(), n + ".h") << "\n";

    auto id = common::scopeFor(m_db.getProtocolNamespace(), common::msgIdEnumName()) + '_' + n;

    auto writeClassDefFunc =
        [this, &out, &id, &n](unsigned ind)
        {
            out << output::indent(ind) << "comms::MessageBase<\n" <<
                   output::indent(ind + 1) << "TMsgBase,\n" <<
                   output::indent(ind + 1) << "comms::option::StaticNumIdImpl<" << id << ">,\n" <<
                   output::indent(ind + 1) << "comms::option::MsgType<" << getReferenceName() << "<TMsgBase, TOpt> >,\n";

            if (m_fields.empty()) {
                out << output::indent(ind + 1) << "comms::option::ZeroFieldsImpl\n";
            }
            else {
                out << output::indent(ind + 1) << "comms::option::FieldsImpl<typename " << n << common::fieldsSuffixStr() << "<TOpt>::All>,\n" <<
                       output::indent(ind + 1) << "comms::option::HasDoRefresh\n";
            }
            out << output::indent(ind) << ">";
        };

    out <<
        "template <typename TMsgBase, typename TOpt = " << common::defaultOptionsStr() << ">\n"
        "class " << getReferenceName() << " : public\n";
    writeClassDefFunc(1);
    out << '\n' <<
           "{\n" <<
           output::indent(1) << "using Base =\n";
    writeClassDefFunc(1);
    out << ";\n\n"
           "public:\n";
    writeFieldsAccess(out);
    writeConstructors(out);
    writeReadFunc(out);
    writeRefreshFunc(out);
    writePrivateMembers(out);
    out << "};\n\n";

    return true;
}

void Message::writeFieldsAccess(std::ostream& out) const
{
    if (m_fields.empty()) {
        return;
    }

    auto& n = getName();
    out <<
        output::indent(1) << "/// \\brief Allow access to internal fields.\n" <<
        output::indent(1) << "/// \\details See definition of \\b COMMS_MSG_FIELDS_ACCESS macro\n" <<
        output::indent(1) << "///     related to \\b comms::MessageBase class from COMMS library\n" <<
        output::indent(1) << "///     for details.\n" <<
        output::indent(1) << "///     \n" <<
        output::indent(1) << "///     The field names are:\n";
    for (auto& f : m_fields) {
        auto& fieldName = f->getName();
        out << output::indent(1) <<
            "///     \\li \\b " << fieldName <<
            " for \\ref " << n << common::fieldsSuffixStr() << "::" <<
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
        auto& fieldName = f->getName();
        out << output::indent(2) << fieldName;
    }
    out << '\n' << output::indent(1) << ");\n\n";
}

bool Message::writeMessageDef(const std::string& filename)
{
    std::ofstream stream(filename);
    if (!stream) {
        log::error() << "Failed to create " << filename;
        return false;
    }

    auto& msgName = getName();
    writeFileHeader(stream, msgName, m_db);
    writeExtraDefHeaders(stream);
    openNamespaces(stream, m_db);
    bool result =
        writeFields(stream) &&
        writeMessageClass(stream);
    closeNamespaces(stream, m_db);
    stream.flush();

    bool written = stream.good();
    if (!written) {
        log::error() << "Failed to write message file" << std::endl;
    }

    return result && written;
}

void Message::writeConstructors(std::ostream& out)
{
    std::string nonBasicFieldName;
    do {
        if (m_fields.empty()) {
            break;
        }

        auto nonBasicFieldIter =
            std::find_if(
                m_fields.begin(), m_fields.end(),
                [](FieldsList::const_reference f)
                {
                    return (f->getKind() != Field::Kind::Basic);
                });

        if (nonBasicFieldIter == m_fields.end()) {
            nonBasicFieldName = "numOfValues";
            break;
        }

        nonBasicFieldName = (*nonBasicFieldIter)->getName();
    } while (false);

    auto& name = getReferenceName();
    out << output::indent(1) << "/// \\brief Default constructor.\n";
    if (nonBasicFieldName.empty()) {
        out << output::indent(1) << name << "() = default;\n\n";
    }
    else {
        out << output::indent(1) << "/// \\details Sets the \\\"blockLength\\\" value.\n" <<
               output::indent(1) << name << "()\n" <<
               output::indent(1) << "{\n" <<
               output::indent(2) << "Base::setBlockLength(Base::template doMaxLengthUntil<FieldIdx_" << nonBasicFieldName << ">());\n" <<
               output::indent(1) << "}\n\n";
    }

    out << output::indent(1) << "/// \\brief Copy constructor.\n" <<
           output::indent(1) << name << "(const " << name << "&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Move constructor.\n" <<
           output::indent(1) << name << "(" << name << "&&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Copy assignment.\n" <<
           output::indent(1) << name << "& operator=(const " << name << "&) = default;\n\n" <<
           output::indent(1) << "/// \\brief Move assignment.\n" <<
           output::indent(1) << name << "& operator=(" << name << "&&) = default;\n\n";
}

void Message::writeReadFunc(std::ostream& out)
{
    out << output::indent(1) << "/// \\brief Custom read functionality.\n" <<
           output::indent(1) << "template <typename TIter>\n" <<
           output::indent(1) << "comms::ErrorStatus doRead(TIter& iter, std::size_t len)\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "GASSERT(Base::getBlockLength() <= len);\n";
    static const std::string advanceStr("std::advance(iter, Base::getBlockLength());\n");
    do {
        if (m_fields.empty()) {
            out << output::indent(2) << "static_cast<void>(len);\n" <<
                   output::indent(2) << advanceStr <<
                   output::indent(2) << "return comms::ErrorStatus::Success;\n";
            break;
        }

        out << output::indent(2) << "updateFieldsVersion();\n";

        auto nonBasicFieldIter =
            std::find_if(
                m_fields.begin(), m_fields.end(),
                [](FieldsList::const_reference f)
                {
                    return (f->getKind() != Field::Kind::Basic);
                });

        if (nonBasicFieldIter == m_fields.begin()) {
            out << output::indent(2) << advanceStr <<
                   output::indent(2) << "return Base::doRead(iter, len - Base::getBlockLength());\n";
            break;
        }

        if (nonBasicFieldIter == m_fields.end()) {
            out << output::indent(2) << "static_cast<void>(len);\n" <<
                   output::indent(2) << "auto iterTmp = iter;\n" <<
                   output::indent(2) << "auto es = Base::doRead(iterTmp, Base::getBlockLength());\n" <<
                   output::indent(2) << "if (es == comms::ErrorStatus::Success) {\n" <<
                   output::indent(3) << advanceStr <<
                   output::indent(2) << "}\n\n" <<
                   output::indent(2) << "return es;\n";
            break;
        }

        auto& fieldName = (*nonBasicFieldIter)->getName();
        out << output::indent(2) << "auto iterTmp = iter;\n" <<
               output::indent(2) << "std::size_t rootBlockLen = Base::getBlockLength();\n" <<
               output::indent(2) << "auto es = Base::template doReadFieldsUntil<FieldIdx_" << fieldName << ">(iterTmp, rootBlockLen);\n" <<
               output::indent(2) << "if (es != comms::ErrorStatus::Success) {\n" <<
               output::indent(3) << "return es;\n" <<
               output::indent(2) << "}\n\n" <<
               output::indent(2) << advanceStr <<
               output::indent(2) << "auto remLen = len - rootBlockLen;\n" <<
               output::indent(2) << "return Base::template doReadFieldsFrom<FieldIdx_" << fieldName << ">(iter, remLen);\n";

    } while (false);
    out << output::indent(1) << "}\n\n";
}

void Message::writeRefreshFunc(std::ostream& out)
{
    if (m_fields.empty()) {
        return;
    }

    out << output::indent(1) << "/// \\brief Custom refresh functionality.\n" <<
           output::indent(1) << "bool doRefresh()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "bool updated = updateFieldsVersion();\n";

    auto nonBasicFieldIter =
        std::find_if(
            m_fields.begin(), m_fields.end(),
            [](FieldsList::const_reference f)
            {
                return (f->getKind() != Field::Kind::Basic);
            });

    do {
        if (nonBasicFieldIter == m_fields.begin()) {
            out << output::indent(2) << "std::size_t currBlockLength = 0U;\n";
            break;
        }

        if (nonBasicFieldIter == m_fields.end()) {
            out << output::indent(2) << "std::size_t currBlockLength = Base::doLength();\n";
            break;
        }

        auto& fieldName = (*nonBasicFieldIter)->getName();
        out << output::indent(2) << "std::size_t currBlockLength = Base::template doLengthUntil<FieldIdx_" << fieldName << ">();\n";
    } while (false);

    out << output::indent(2) << "if (currBlockLength == Base::getBlockLength()) {\n" <<
           output::indent(3) << "return updated;\n" <<
           output::indent(2) << "}\n\n" <<
           output::indent(2) << "Base::setBlockLength(currBlockLength);\n" <<
           output::indent(2) << "return true;\n" <<
           output::indent(1) << "}\n\n";
}

void Message::writePrivateMembers(std::ostream& out)
{
    if (m_fields.empty()) {
        return;
    }

    out << "private:\n" <<
           output::indent(1) << "bool updateFieldsVersion()\n" <<
           output::indent(1) << "{\n" <<
           output::indent(2) << "return comms::util::tupleAccumulate(Base::fields(), false, " <<
                                common::builtinNamespaceStr() << common::versionSetterStr() << "(Base::getVersion()));\n" <<
           output::indent(1) << "}\n;";
}

void Message::writeExtraDefHeaders(std::ostream& out)
{
    std::set<std::string> extraHeaders;
    extraHeaders.insert("<iterator>");
    extraHeaders.insert("\"comms/MessageBase.h\"");
    extraHeaders.insert("\"comms/Assert.h\"");
    extraHeaders.insert('\"' + napespacePrefix(m_db) + common::defaultOptionsFileName() + '\"');
    extraHeaders.insert('\"' + napespacePrefix(m_db) + common::msgIdFileName() + '\"');

    for (auto& f : m_fields) {
        f->updateExtraHeaders(extraHeaders);
    }

    if (!m_fields.empty()) {
        extraHeaders.insert(common::localHeader(m_db.getProtocolNamespace(), common::builtinNamespaceNameStr(), common::versionSetterFileName()));
        extraHeaders.insert("\"comms/util/Tuple.h\"");
    }

    common::writeExtraHeaders(out, extraHeaders);
}

bool Message::writeProtocolDef()
{
    if (!common::createProtocolDefDir(m_db.getRootPath(), m_db.getProtocolNamespace(), common::messageDirName())) {
        return false;
    }

    auto messageDirRelPath =
            common::protocolDirRelPath(m_db.getProtocolNamespace(), common::messageDirName());

    const std::string Ext(".h");
    auto filename = getName() + Ext;
    auto relPath = bf::path(messageDirRelPath) / filename;
    auto filePath = bf::path(m_db.getRootPath()) / relPath;

    log::info() << "Generating " << relPath.string() << std::endl;
    return writeMessageDef(filePath.string());
}

bool Message::writePluginHeader()
{
    if (!common::createPluginDefDir(m_db.getRootPath(), common::messageDirName())) {
        return false;
    }

    auto& ns = common::pluginNamespaceNameStr();
    auto relPath = common::pathTo(ns, common::messageDirName() + '/' + getName() + ".h");
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& protNs = m_db.getProtocolNamespace();
    out << "#pragma once\n\n"
           "#include \"comms_champion/comms_champion.h\"\n"
           "#include \"cc_plugin/" << common::msgInterfaceFileName() << "\"\n"
           "#include " << common::localHeader(protNs, common::messageNamespaceNameStr(), getName() + ".h") << "\n\n";

    openPluginNamespaces(out, m_db);

    auto protMsgScope = common::scopeFor(protNs, common::messageNamespaceStr() + getName());
    auto pluginInterfaceScope = common::scopeFor(protNs, common::pluginNamespaceStr() + common::msgInterfaceStr());

    out << "class " << getReferenceName() << " : public\n" <<
           output::indent(1) << "comms_champion::ProtocolMessageBase<\n" <<
           output::indent(2) << protMsgScope << '<' << pluginInterfaceScope << "<> >,\n" <<
           output::indent(2) << getReferenceName() << ">\n" <<
           "{\n"
           "protected:\n" <<
           output::indent(1) << "virtual const char* nameImpl() const override;\n" <<
           output::indent(1) << "virtual const QVariantList& fieldsPropertiesImpl() const override;\n" <<
           "};\n\n";
    closePluginNamespaces(out, m_db);
    return true;

}

bool Message::writePluginSrc()
{
    if (!common::createPluginDefDir(m_db.getRootPath(), common::messageDirName())) {
        return false;
    }

    auto& ns = common::pluginNamespaceNameStr();
    auto relPath = common::pathTo(ns, common::messageDirName() + '/' + getName() + ".cpp");
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "#include \"" << getName() << ".h\"\n\n"
           "#include <cassert>\n"
           "#include <QtCore/QVariantList>\n"
           "#include \"cc_plugin/" << common::fieldHeaderFileName() << "\"\n\n";

    openPluginNamespaces(out, m_db);

    out << "namespace\n"
           "{\n\n";

    static const std::string createFieldPropsFuncPrefix("createFieldProps_");
    auto relScope = common::messageNamespaceStr() + getName() + common::fieldsSuffixStr() + "<>::";
    auto scope = common::scopeFor(m_db.getProtocolNamespace(), relScope);
    for (auto& f : m_fields) {
        out << "QVariantMap " << createFieldPropsFuncPrefix << f->getName() << "()\n"
               "{\n";
        if (!f->writePluginProperties(out, 1, scope)) {
            return false;
        }
        out << "}\n\n";

    }

    static const std::string createPropertiesFuncName("createFieldsProperties");
    out << "QVariantList " << createPropertiesFuncName << "()\n"
           "{\n" <<
           output::indent(1) << "QVariantList props;\n";
    for (auto& f : m_fields) {
        out << output::indent(1) << "props.append(" << createFieldPropsFuncPrefix << f->getName() << "());\n";
    }

    out << '\n';
    if (!m_fields.empty()) {
        out << output::indent(1) << "assert(props.size() == " << getReferenceName() << "::FieldIdx_numOfValues);\n";
    }

    out << output::indent(1) << "return props;\n"
           "}\n\n"
           "} // namespace\n\n"
           "const char* " << getReferenceName() << "::nameImpl() const\n"
           "{\n" <<
           output::indent(1) << "static const char* Str = \"" << getName() << "\";\n" <<
           output::indent(1) << "return Str;\n"
           "}\n\n"
           "const QVariantList& " << getReferenceName() << "::fieldsPropertiesImpl() const\n"
           "{\n" <<
           output::indent(1) << "static const auto Props = " << createPropertiesFuncName << "();\n" <<
           output::indent(1) << "return Props;\n"
           "}\n\n";

    closePluginNamespaces(out, m_db);
    return true;

}



} // namespace sbe2comms
