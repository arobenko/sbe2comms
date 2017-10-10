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
        makePairFunc("float"),
        makePairFunc("double"),
        makePairFunc("int"),
        makePairFunc("unsigned"),
        makePairFunc("class"),
        makePairFunc("struct"),
        makePairFunc("void"),
        makePairFunc("long"),
        makePairFunc("short")
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
           output::indent(indent + 1) << "return Base::value() == static_cast<Base::ValueType>(" << val << ");\n" <<
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

void recordExtraHeader(const std::string& newHeader, std::set<std::string>& allHeaders)
{
    auto iter = allHeaders.find(newHeader);
    if (iter != allHeaders.end()) {
        return;
    }
    allHeaders.insert(newHeader);
}

} // namespace common

} // namespace sbe2comms
