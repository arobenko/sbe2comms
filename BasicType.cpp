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

#include "BasicType.h"

#include <map>
#include <iterator>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>

#include "prop.h"
#include "get.h"
#include "output.h"

namespace sbe2comms
{

namespace
{

const std::string CharType("char");

const std::string& primitiveFloatToStd(const std::string& type)
{
    static const std::string Values[] = {
        "float", "double"
    };

    auto iter = std::find(std::begin(Values), std::end(Values), type);
    if (iter == std::end(Values)) {
        return get::emptyString();
    }

    return *iter;
}

} // namespace

BasicType::Kind BasicType::kindImpl() const
{
    return Kind::Basic;
}

bool BasicType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    writeBrief(out, db, indent);

    auto& p = props(db);

    bool result = false;
    do {
        auto& primType = prop::primitiveType(p);
        if (primType.empty()) {
            std::cerr << "ERROR: Primitive type was not provided for type \"" << prop::name(p) << "\"" << std::endl;
            out << get::unknownValueString();
            break;
        }

        unsigned length = prop::length(p);
        if ((length == 1) && (!isConstString(db))) {
            result = writeSimpleType(out, db, indent, primType);
            break;
        }

        if (prop::isOptional(p)) {
            std::cerr << "ERROR: The \"optional\" fields may NOT have non-default length. "
                         "See definition of \"" << prop::name(p) << "\" type." << std::endl;
            return false;
        }

        if (length == 0) {
            result = writeVarLength(out, db, indent, primType);
            break;
        }

        result = writeFixedLength(out, db, indent, primType);

    } while (false);
    out << ";\n\n";
    return result;
}

std::size_t BasicType::lengthImpl(DB& db)
{
    auto& p = props(db);
    if (prop::isConstant(p)) {
        return 0U;
    }

    auto len = prop::length(p);
    if (len == 0) {
        return 0U;
    }

    auto& primType = prop::primitiveType(p);
    if (primType.empty()) {
        return 0U;
    }

    if (prop::isConstant(p)) {
        return 0U;
    }

    auto singleLen = primitiveLength(primType);
    return singleLen * len;
}

bool BasicType::writeSimpleType(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType,
    bool embedded)
{
    auto& intType = primitiveTypeToStdInt(primType);
    if (!intType.empty()) {
        return writeSimpleInt(out, db, indent, intType, embedded);
    }

    auto& fpType = primitiveFloatToStd(primType);
    if (!fpType.empty()) {
        return writeSimpleFloat(out, db, indent, fpType, embedded);
    }

    std::cerr << "ERROR: Unknown primitiveType \"" << primType << "\" for "
                 "field \"" << prop::name(props(db)) << '\"' << std::endl;
    out << get::unknownValueString() << "\n\n";
    return false;
}

bool BasicType::writeSimpleInt(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& intType,
    bool embedded)
{
    bool result = false;
    do {
        auto& p = props(db);
        auto& primType = prop::primitiveType(p);
        assert (!primType.empty());
        auto minValStr = prop::minValue(p);
        auto minVal = intMinValue(primType, minValStr);
        if (!minVal.second) {
            std::cerr << "ERROR: invalid minValue attribute \"" << minValStr << "\" for type \"" <<
                         prop::name(p) << "\"." << std::endl;
            break;
        }

        auto maxValStr = prop::maxValue(p);
        auto maxVal = intMaxValue(primType, maxValStr);
        if (!maxVal.second) {
            std::cerr << "ERROR: invalid maxValue attribute \"" << maxValStr << "\" for type \"" <<
                         prop::name(p) << "\"." << std::endl;
            break;
        }

        if (maxVal.first < minVal.first) {
            std::cerr << "ERROR: min/max values range error for type \"" << prop::name(p) << "\"." << std::endl;
            break;
        }

        std::intmax_t defValue = 0;
        defValue = std::min(std::max(defValue, minVal.first), maxVal.first);
        bool constant = false;
        boost::optional<std::intmax_t> extraValidNumber;

        auto writeFunc =
            [this, intType, &defValue, &constant, &minVal, &maxVal, &extraValidNumber, &out](unsigned ind)
            {
                out << output::indent(ind) << "comms::field::IntValue<\n" <<
                       output::indent(ind + 1) << "FieldBase,\n" <<
                       output::indent(ind + 1) << intType << ",\n";
                if (minVal.first != maxVal.first) {
                    out << output::indent(ind + 1) << "comms::option::ValidNumValueRange<" <<
                                toString(minVal.first) << ", " << toString(maxVal.first) << ">";
                }
                else {
                    out << output::indent(ind + 1) << "comms::option::ValidNumValue<" <<
                                toString(minVal.first) << ">";
                }

                if (extraValidNumber) {
                    out << ",\n" <<
                           output::indent(ind + 1) << "comms::option::ValidNumValue<" <<
                                toString(*extraValidNumber) << ">";
                }

                if (defValue != 0) {
                    out << ",\n" <<
                           output::indent(ind + 1) << "comms::option::DefaultNumValue<" << toString(defValue) << ">";
                }
                if (constant) {
                    out << ",\n" <<
                           output::indent(ind + 1) << "comms::option::EmptySerialization";
                }
                out << "\n" << output::indent(ind) << ">";
            };

        if (prop::isRequired(p)) {
            if (!embedded) {
                out << output::indent(indent) << "using " << prop::name(p) << " = \n";
            }
            writeFunc(indent + 1);
            result = true;
            break;
        }

        if (prop::isConstant(p)) {
            assert(!embedded);
            constant = true;
            auto text = nodeText();
            if (text.empty()) {
                std::cerr << "ERROR: Empty constant value for type \"" << prop::name(p) << "\"." << std::endl;
                break;
            }

            try {
                defValue = std::stoll(text);
                minVal.first = defValue;
                maxVal.first = defValue;
            } catch (...) {
                std::cerr << "ERROR: Invalid constant value \"" << text << "\" for type \"" << prop::name(p) << "\"." << std::endl;
                break;
            }

            out << output::indent(indent) << "using " << prop::name(p) << " = \n";
            writeFunc(indent + 1);
            result = true;
            break;
        }

        if (prop::isOptional(p)) {
            auto& nullValStr = prop::nullValue(p);
            std::intmax_t nullValue = 0;
            if (nullValStr.empty()) {
                nullValue = builtInIntNullValue(intType);
            }
            else {
                auto convertResult = stringToInt(nullValStr);
                if (!convertResult.second) {
                    std::cerr << "ERROR: Bad nullValue for type \"" << prop::name(p) << "\": " << nullValStr << std::endl;
                    break;
                }
                nullValue = convertResult.first;
            }

            extraValidNumber = nullValue;
            out << output::indent(indent) << "struct " << prop::name(p) << " : public\n";
            writeFunc(indent + 1);
            out << output::indent(indent) << "\n" <<
                   output::indent(indent) << "{\n" <<
                   output::indent(indent + 1) << "static const " << intType << " NullValue = static_cast<" << intType << ">(" << nullValue << ");\n" <<
                   output::indent(indent) << "}";
            result = true;
            break;
        }

        std::cerr << "ERROR: Unkown \"presence\" token value \"" << prop::presence(p) << "\"." << std::endl;
    } while (false);

    if (!result) {
        out << get::unknownValueString();
    }

    return result;
}

bool BasicType::writeSimpleFloat(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& fpType,
    bool embedded)
{
    if (!embedded) {
        auto p = props(db);
        out << output::indent(indent) << "using " << prop::name(p) << " = \n";
    }

    out << output::indent(indent + 1) << "comms::field::FloatValue<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << fpType << "\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeVarLength(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    auto& p = props(db);
    if (prop::isConstant(p)) {
        std::cerr << "ERROR: Variable lengths constant types are unsupported (\"" << prop::name(p) << "\")." << std::endl;
        return false;
    }
    
    if (isString(db)) {
        return writeVarLengthString(out, db, indent);
    }

    return writeVarLengthArray(out, db, indent, primType);
}


bool BasicType::writeVarLengthString(
    std::ostream& out,
    DB& db,
    unsigned indent)
{
    auto p = props(db);
    out << output::indent(indent) << "using " << prop::name(p) << " = comms::field::String<FieldBase>";
    return true;
}

bool BasicType::writeVarLengthArray(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    static const std::string RawDataTypes[] = {
        CharType,
        "int8",
        "uint8"
    };

    auto iter = std::find(std::begin(RawDataTypes), std::end(RawDataTypes), primType);
    if (iter != std::end(RawDataTypes)) {
        return writeVarLengthRawDataArray(out, db, indent, primType);
    }

    auto p = props(db);
    out << output::indent(indent) << "using " << prop::name(p) << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << "FieldBase,\n";
    writeSimpleType(out, db, indent + 1, primType, true);
    out << "\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeVarLengthRawDataArray(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    auto p = props(db);
    out << output::indent(indent) << "using " << prop::name(p) << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << primitiveTypeToStdInt(primType) << "\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeFixedLength(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    if (isString(db)) {
        return writeFixedLengthString(out, db, indent);
    }
    
    auto& p = props(db);
    if (prop::isConstant(p)) {
        std::cerr << "ERROR: Fixed length constant arrays are unsupported (\"" << prop::name(p) << "\")." << std::endl;
        return false;
    }

    return writeFixedLengthArray(out, db, indent, primType);
}

bool BasicType::writeFixedLengthString(
    std::ostream& out,
    DB& db,
    unsigned indent)
{
    auto& p = props(db);
    if (!isConstString(db)) {
        unsigned len = prop::length(p);
        assert(1U < len);
        out << output::indent(indent) << "using " << prop::name(p) << " = \n" <<
               output::indent(indent + 1) << "comms::field::String<\n" <<
               output::indent(indent + 2) << "FieldBase,\n" <<
               output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << len << ">\n" <<
               output::indent(indent + 1) << ">";
        return true;
    }
    
    auto text = xmlText(getNode());
    out << output::indent(indent) << "struct " << prop::name(p) << " : public \n" <<
           output::indent(indent + 1) << "comms::field::String<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << "comms::option::EmptySerialization\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << prop::name(p) << "()\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "using Base = typename std::decay<decltype(toFieldBase(*this))>::type;\n" <<
           output::indent(indent + 2) << "static const char Chars[" << text.size() << "] = {\n" <<
           output::indent(indent + 3);
    bool firstChar = true;
    for (auto ch : text) {
        if (!firstChar) {
            out << ", ";
        }

        firstChar = false;
        out << '\'' << ch << '\'';
    }
    out << '\n' <<
           output::indent(indent + 2) << "}\n\n" <<
           output::indent(indent + 2) << "Base::value() = Chars;\n" <<
           output::indent(indent + 1) << "}\n" <<
           output::indent(indent) << "}";
    
    return true;
}

bool BasicType::writeFixedLengthArray(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    static const std::string RawDataTypes[] = {
        CharType,
        "int8",
        "uint8"
    };

    auto iter = std::find(std::begin(RawDataTypes), std::end(RawDataTypes), primType);
    if (iter != std::end(RawDataTypes)) {
        return writeFixedLengthRawDataArray(out, db, indent, primType);
    }

    auto& p = props(db);
    unsigned len = prop::length(p);
    assert(1U < len);

    out << output::indent(indent) << "using " << prop::name(p) << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << "FieldBase,\n";
    writeSimpleType(out, db, indent + 1, primType, true);
    out << ",\n" <<
           output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << len << ">\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeFixedLengthRawDataArray(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    auto& p = props(db);
    unsigned len = prop::length(p);
    assert(1U < len);

    out << output::indent(indent) << "using " << prop::name(p) << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << primitiveTypeToStdInt(primType) << ",\n" <<
           output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << len << ">\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::hasMinMaxValues(DB& db)
{
    auto& p = props(db);
    return prop::hasMinValue(p) || prop::hasMaxValue(p);
}

bool BasicType::isString(DB& db)
{
    static const std::string StringTypes[] = {
        CharType,
        "int8",
        "uint8"
    };

    auto& p = props(db);
    auto& primType = prop::primitiveType(p);
    auto iter = std::find(std::begin(StringTypes), std::end(StringTypes), primType);
    if (iter == std::end(StringTypes)) {
        return false;
    }

    auto semType = prop::semanticType(p); // by value
    boost::algorithm::to_lower(semType);
    static const std::string StringSemanticType("string");
    if (semType == StringSemanticType) {
        return true;
    }

    return primType == CharType;
}

bool BasicType::isConstString(DB& db)
{
    auto& p = props(db);
    return prop::isConstant(p) && isString(db);
}


} // namespace sbe2comms
