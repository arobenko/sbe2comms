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

#include "common.h"

#include <map>
#include <iostream>
#include <limits>

#include "output.h"

namespace sbe2comms
{

namespace common
{

const std::string& messageDirName()
{
    static const std::string Name("message");
    return Name;
}

const std::string& includeDirName()
{
    static const std::string Name("include");
    return Name;
}

const std::string& fieldsDefFileName()
{
    static const std::string Name("field.h");
    return Name;
}

const std::string& builtinsDefFileName()
{
    static const std::string Name("sbe2comms.h");
    return Name;
}

const std::string& defaultOptionsFileName()
{
    static const std::string Name(defaultOptionsStr() + ".h");
    return Name;
}

const std::string& msgIdFileName()
{
    static const std::string Name(msgIdEnumName() + ".h");
    return Name;
}

const std::string& msgInterfaceFileName()
{
    static const std::string Name("Message.h");
    return Name;
}

const std::string& defaultOptionsStr()
{
    static const std::string Str("DefaultOptions");
    return Str;
}

const std::string& emptyString()
{
    static const std::string Str;
    return Str;
}

const std::string& renameKeyword(const std::string& value)
{
    auto makePairFunc =
        [](const char* s)
        {
            std::string str(s);
            std::string sub(str + '_');
            return make_pair(std::move(str), std::move(sub));
        };

    static const std::map<std::string, std::string> Keywords = {
        makePairFunc("alignas"),
        makePairFunc("alignof"),
        makePairFunc("and"),
        makePairFunc("and_eq"),
        makePairFunc("atomic_cancel"),
        makePairFunc("atomic_commit"),
        makePairFunc("atomic_noexcept"),
        makePairFunc("auto"),
        makePairFunc("bitand"),
        makePairFunc("bitor"),
        makePairFunc("bool"),
        makePairFunc("break"),
        makePairFunc("case"),
        makePairFunc("catch"),
        makePairFunc("char"),
        makePairFunc("char16_t"),
        makePairFunc("char32_t"),
        makePairFunc("class"),
        makePairFunc("alignas"),
        makePairFunc("compl"),
        makePairFunc("concept"),
        makePairFunc("const"),
        makePairFunc("constexpr"),
        makePairFunc("const_cast"),
        makePairFunc("continue"),
        makePairFunc("co_await"),
        makePairFunc("co_return"),
        makePairFunc("co_yield"),
        makePairFunc("decltype"),
        makePairFunc("default"),
        makePairFunc("delete"),
        makePairFunc("do"),
        makePairFunc("double"),
        makePairFunc("dynamic_cast"),
        makePairFunc("else"),
        makePairFunc("enum"),
        makePairFunc("explicit"),
        makePairFunc("export"),
        makePairFunc("extern"),
        makePairFunc("false"),
        makePairFunc("float"),
        makePairFunc("for"),
        makePairFunc("friend"),
        makePairFunc("goto"),
        makePairFunc("if"),
        makePairFunc("import"),
        makePairFunc("inline"),
        makePairFunc("int"),
        makePairFunc("long"),
        makePairFunc("module"),
        makePairFunc("mutable"),
        makePairFunc("namespace"),
        makePairFunc("new"),
        makePairFunc("noexcept"),
        makePairFunc("not"),
        makePairFunc("not_eq"),
        makePairFunc("nullptr"),
        makePairFunc("operator"),
        makePairFunc("or"),
        makePairFunc("or_eq"),
        makePairFunc("private"),
        makePairFunc("protected"),
        makePairFunc("public"),
        makePairFunc("register"),
        makePairFunc("reinterpret_cast"),
        makePairFunc("requires"),
        makePairFunc("return"),
        makePairFunc("short"),
        makePairFunc("signed"),
        makePairFunc("sizeof"),
        makePairFunc("static"),
        makePairFunc("static_assert"),
        makePairFunc("static_cast"),
        makePairFunc("struct"),
        makePairFunc("switch"),
        makePairFunc("synchronized"),
        makePairFunc("template"),
        makePairFunc("this"),
        makePairFunc("thread_local"),
        makePairFunc("throw"),
        makePairFunc("true"),
        makePairFunc("try"),
        makePairFunc("typedef"),
        makePairFunc("typeid"),
        makePairFunc("typename"),
        makePairFunc("union"),
        makePairFunc("unsigned"),
        makePairFunc("using"),
        makePairFunc("virtual"),
        makePairFunc("void"),
        makePairFunc("volatile"),
        makePairFunc("wchar_t"),
        makePairFunc("while"),
        makePairFunc("xor"),
        makePairFunc("xor_eq"),

        // used namespaces
        makePairFunc("std"),
        makePairFunc("field"),
        makePairFunc("message"),
        makePairFunc("sbe2comms"),
    };

    auto iter = Keywords.find(value);
    if (iter == Keywords.end()) {
        return value;
    }

    return iter->second;
}

const std::string& extraOptionsDocStr()
{
    static const std::string Str("/// \\tparam TOpt Extra options from \\b comms::option namespace.\n");
    return Str;
}

const std::string& elementSuffixStr()
{
    static const std::string Str("Element");
    return Str;
}

const std::string& extraOptionsTemplParamStr()
{
    static const std::string Str("template <typename... TOpt>\n");
    return Str;
}

const std::string& fieldBaseStr()
{
    static const std::string Str("FieldBase");
    return Str;
}

const std::string& fieldBaseDefStr()
{
    static const std::string Str("using Base = typename std::decay<decltype(toFieldBase(*this))>::type;\n");
    return Str;
}

const std::string& messageBaseDefStr()
{
    static const std::string Str("using Base = typename std::decay<decltype(toMessageBase(*this))>::type;\n");
    return Str;
}

const std::string& enumValSuffixStr()
{
    static const std::string Str("Val");
    return Str;
}

const std::string& enumNullValueStr()
{
    static const std::string Str("NullValue");
    return Str;
}

const std::string& fieldNamespaceStr()
{
    static const std::string Str(fieldNamespaceNameStr() + "::");
    return Str;
}

const std::string& fieldNamespaceNameStr()
{
    static const std::string Str("field");
    return Str;
}

const std::string& messageNamespaceStr()
{
    static const std::string Str(messageNamespaceNameStr() + "::");
    return Str;
}

const std::string& messageNamespaceNameStr()
{
    static const std::string Str("message");
    return Str;
}

const std::string& builtinNamespaceStr()
{
    static const std::string Str("sbe2comms::");
    return Str;
}

const std::string& memembersSuffixStr()
{
    static const std::string Str("Members");
    return Str;
}

const std::string& fieldsSuffixStr()
{
    static const std::string Str("Fields");
    return Str;
}

const std::string& eqEmptyOptionStr()
{
    static const std::string Str(" = comms::option::EmptyOption");
    return Str;
}

const std::string& optParamPrefixStr()
{
    static const std::string Str("typename TOpt::");
    return Str;
}

const std::string& blockLengthStr()
{
    static const std::string Str("blockLength");
    return Str;
}

const std::string& numInGroupStr()
{
    static const std::string Str("numInGroup");
    return Str;
}

const std::string& groupListStr()
{
    static const std::string Str("groupList");
    return Str;
}

const std::string& templateIdStr()
{
    static const std::string Str("templateId");
    return Str;
}

const std::string& schemaIdStr()
{
    static const std::string Str("schemaId");
    return Str;
}

const std::string& versionStr()
{
    static const std::string Str("version");
    return Str;
}

const std::string& msgIdEnumName()
{
    static const std::string Str("MsgId");
    return Str;
}

std::string num(std::intmax_t val)
{
    auto str = std::to_string(val);
    if ((std::numeric_limits<std::int32_t>::max() < val) || (val < std::numeric_limits<std::int32_t>::min())) {
        str += "LL";
        return str;
    }

    if ((std::numeric_limits<std::int16_t>::max() < val) || (val < std::numeric_limits<std::int16_t>::min())) {
        str += "L";
        return str;
    }

    return str;
}

std::string scopeFor(const std::string& ns, const std::string type)
{
    std::string result = ns;
    if (!ns.empty()) {
        result += "::";
    }
    result += type;
    return result;
}

std::string pathTo(const std::string& ns, const std::string path)
{
    std::string result = ns;
    if (!ns.empty()) {
        result += '/';
    }
    result += path;
    return result;
}

const std::string& primitiveTypeToStdInt(const std::string& type)
{
    static const std::map<std::string, std::string> Map = {
        std::make_pair("char", "char"),
        std::make_pair("int8", "std::int8_t"),
        std::make_pair("uint8", "std::uint8_t"),
        std::make_pair("int16", "std::int16_t"),
        std::make_pair("uint16", "std::uint16_t"),
        std::make_pair("int32", "std::int32_t"),
        std::make_pair("uint32", "std::uint32_t"),
        std::make_pair("int64", "std::int64_t"),
        std::make_pair("uint64", "std::uint64_t")
    };

    auto iter = Map.find(type);
    if (iter == Map.end()) {
        return common::emptyString();
    }

    return iter->second;
}

void writeDetails(std::ostream& out, unsigned indent, const std::string& desc)
{
    if (desc.empty()) {
        return;
    }

    out << output::indent(indent) << "/// \\details " << desc << "\n";
}

void writeExtraOptionsDoc(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << extraOptionsDocStr();
}

void writeExtraOptionsTemplParam(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << extraOptionsTemplParamStr();
}

void writeIntIsNullFunc(std::ostream& out, unsigned indent, std::intmax_t val)
{
    out << output::indent(indent) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
           output::indent(indent) << "bool isNull() const\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << fieldBaseDefStr() <<
           output::indent(indent + 1) << "return Base::value() == static_cast<Base::ValueType>(" << num(val) << ");\n" <<
           output::indent(indent) << "}\n";

}

void writeFpIsNullFunc(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
           output::indent(indent) << "bool isNull() const\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << fieldBaseDefStr() <<
           output::indent(indent + 1) << "return std::isnan(Base::value());\n" <<
           output::indent(indent) << "}\n";

}

void writeFpOptConstructor(
    std::ostream& out,
    unsigned indent,
    const std::string& name,
    const std::string& customDefault)
{
    out << output::indent(indent) << "/// \\brief Default constructor.\n" <<
           output::indent(indent) << "/// \\details Initializes field's value to ";
    if (customDefault.empty()) {
        out << "NaN";
    }
    else {
        out << customDefault;
    }
    out << '\n' <<
           output::indent(indent) << name << "::" << name << "()\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << fieldBaseDefStr() <<
           output::indent(indent + 1) << "Base::value() = ";
    if (customDefault.empty()) {
        out << "std::numeric_limits<typename Base::ValueType>::quiet_NaN()";
    }
    else {
        out << "static_cast<Base::ValueType>(" << customDefault << ')';
    }
    out << ";\n" <<
           output::indent(indent) << "}\n";
}

void writeFpValidCheckFunc(std::ostream& out, unsigned indent, bool nanValid)
{
    out << output::indent(indent) << "/// \\brief Value validity check function.\n" <<
           output::indent(indent) << "bool valid() const\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << common::fieldBaseDefStr() <<
           output::indent(indent + 1) << "return Base::valid()";
    if (!nanValid) {
        out << " && (!std::isnan(Base::value()))";
    }
    out << ";\n" <<
           output::indent(indent) << "}\n";
}


void writeEnumIsNullFunc(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
           output::indent(indent) << "bool isNull() const\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << fieldBaseDefStr() <<
           output::indent(indent + 1) << "return Base::value() == Base::ValueType::" << enumNullValueStr() << ";\n" <<
           output::indent(indent) << "}\n";
}

void recordExtraHeader(const std::string& newHeader, std::set<std::string>& allHeaders)
{
    auto iter = allHeaders.find(newHeader);
    if (iter != allHeaders.end()) {
        return;
    }
    allHeaders.insert(newHeader);
}

void writeProtocolNamespaceBegin(const std::string& ns, std::ostream& out)
{
    if (ns.empty()) {
        return;
    }

    out << "namespace " << ns << "\n"
           "{\n"
           "\n";
}

void writeProtocolNamespaceEnd(const std::string& ns, std::ostream& out)
{
    if (ns.empty()) {
        return;
    }

    out << "} // namespace " << ns << "\n\n";
}

} // namespace common

} // namespace sbe2comms
