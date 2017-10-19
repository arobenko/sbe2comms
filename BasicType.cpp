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

#include "log.h"
#include "prop.h"
#include "common.h"
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
        return common::emptyString();
    }

    return *iter;
}


} // namespace

const std::string& BasicType::getPrimitiveType() const
{
    return prop::primitiveType(getProps());
}

std::intmax_t BasicType::getDefultIntNullValue() const
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    auto& intType = primitiveTypeToStdInt(primType);
    assert(!intType.empty());
    return builtInIntNullValue(intType);
}

bool BasicType::isIntType() const
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    auto& intType = primitiveTypeToStdInt(primType);
    return !intType.empty();
}

bool BasicType::isFpType() const
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    auto& fpType = primitiveFloatToStd(primType);
    return !fpType.empty();
}

BasicType::Kind BasicType::getKindImpl() const
{
    return Kind::Basic;
}

bool BasicType::parseImpl()
{
    auto& primType = getPrimitiveType();
    if (primType.empty()) {
        log::error() << "Primitive type was not provided for type \"" << getName() << "\"." << std::endl;
        return false;
    }

    if (isConstant()) {
        auto text = xmlText(getNode());
        if (text.empty()) {
            log::error() << "No constant value provided for type \"" << getName() << "\"." << std::endl;
            return false;
        }

        auto len = getLengthProp();
        if (len != 1U) {
            log::error() << "Constant type \"" << getName() << "\" can NOT have non default length property." << std::endl;
            return false;
        }
    }

    auto& fpType = primitiveFloatToStd(getPrimitiveType());
    if (!fpType.empty()) {
        addExtraInclude("<limits>");
        addExtraInclude("<cmath>");
    }

    return true;
}

bool BasicType::writeImpl(std::ostream& out, unsigned indent)
{
    auto len = getLengthProp();
    if ((len != 1) && (!isString()) && (!isRawData())) {
        writeElementHeader(out, indent);
        common::writeExtraOptionsTemplParam(out, indent);
        writeSimpleType(out, indent, true);
        out << ";\n\n";
    }

    writeHeader(out, indent);
    common::writeExtraOptionsTemplParam(out, indent);
    bool result = false;
    do {

        if ((len == 1) && (!isConstString())) {
            result = writeSimpleType(out, indent);
            break;
        }

        if (len == 0) {
            result = writeVarLength(out, indent);
            break;
        }

        result = writeFixedLength(out, indent);

    } while (false);
    out << ";\n\n";
    return result;
}

std::size_t BasicType::getSerializationLengthImpl() const
{
    if (isConstant()) {
        return false;
    }

    auto count = getLengthProp();
    auto singleLen = primitiveLength(getPrimitiveType());
    return singleLen * count;
}\

bool BasicType::hasFixedLengthImpl() const
{
    return (getLengthProp() != 0U);
}

bool BasicType::writeSimpleType(std::ostream& out,
    unsigned indent,
    bool isElement)
{
    auto& primType = getPrimitiveType();
    auto& intType = primitiveTypeToStdInt(primType);
    if (!intType.empty()) {
        return writeSimpleInt(out, indent, intType, isElement);
    }

    auto& fpType = primitiveFloatToStd(primType);
    if (!fpType.empty()) {
        return writeSimpleFloat(out, indent, fpType, isElement);
    }

    log::error() << "Unknown primitiveType \"" << primType << "\" for "
                 "field \"" << getName() << '\"' << std::endl;
    return false;
}

bool BasicType::writeSimpleInt(std::ostream& out,
    unsigned indent,
    const std::string& intType,
    bool isElement)
{
    bool result = false;
    do {
        auto& primType = getPrimitiveType();
        auto& minValStr = getMinValue();
        auto minVal = intMinValue(primType, minValStr);
        if (!minVal.second) {
            log::error() << "Invalid minValue attribute \"" << minValStr << "\" for type \"" <<
                         getName() << "\"." << std::endl;
            break;
        }

        auto& maxValStr = getMaxValue();
        auto maxVal = intMaxValue(primType, maxValStr);
        if (!maxVal.second) {
            log::error() << "Invalid maxValue attribute \"" << maxValStr << "\" for type \"" <<
                         getName() << "\"." << std::endl;
            break;
        }

        if (maxVal.first < minVal.first) {
            log::error() << "min/max values range error for type \"" << getName() << "\"." << std::endl;
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
                       output::indent(ind + 1) << common::fieldBaseStr() << ",\n" <<
                       output::indent(ind + 1) << intType << ",\n";
                writeExtraOptions(out, ind + 1);

                if (minVal.first != maxVal.first) {
                    out << output::indent(ind + 1) << "comms::option::ValidNumValueRange<" <<
                                common::num(minVal.first) << ", " << common::num(maxVal.first) << ">,\n";
                }
                else {
                    out << output::indent(ind + 1) << "comms::option::ValidNumValue<" <<
                                common::num(minVal.first) << ">,\n";
                }

                if (extraValidNumber) {
                    out << output::indent(ind + 1) << "comms::option::ValidNumValue<" <<
                                common::num(*extraValidNumber) << ">,\n";
                }

                if ((defValue != 0) && (!hasDefaultValueInExtraOptions())) {
                    out << output::indent(ind + 1) << "comms::option::DefaultNumValue<" << common::num(defValue) << ">";
                }

                if (constant) {
                    out << output::indent(ind + 1) << "comms::option::EmptySerialization";
                }

                writeFailOnInvalid(out, ind + 1);

                out << output::indent(ind + 1) << "TOpt...\n" <<
                       output::indent(ind) << ">";
            };

        std::string name;
        if (isElement) {
            name = getName() + common::elementSuffixStr();
        }
        else {
            name = getReferenceName();
        }

        if (isRequired()) {
            out << output::indent(indent) << "using " << name << " = \n";
            writeFunc(indent + 1);
            result = true;
            break;
        }

        if (isConstant()) {
            assert(!isElement);
            constant = true;
            auto text = nodeText();
            if (text.empty()) {
                log::error() << "Empty constant value for type \"" << getName() << "\"." << std::endl;
                break;
            }

            try {
                defValue = std::stoll(text);
                minVal.first = defValue;
                maxVal.first = defValue;
            } catch (...) {
                log::error() << "Invalid constant value \"" << text << "\" for type \"" << getName() << "\"." << std::endl;
                break;
            }

            out << output::indent(indent) << "using " << name << " = \n";
            writeFunc(indent + 1);
            result = true;
            break;
        }

        if (isOptional()) {
            auto& nullValStr = getNullValue();
            std::intmax_t nullValue = 0;
            if (nullValStr.empty()) {
                nullValue = builtInIntNullValue(intType);
            }
            else {
                auto convertResult = stringToInt(nullValStr);
                if (!convertResult.second) {
                    log::error() << "ERROR: Bad nullValue for type \"" << getName() << "\": " << nullValStr << std::endl;
                    break;
                }
                nullValue = convertResult.first;
            }
            defValue = nullValue;

            extraValidNumber = nullValue;
            out << output::indent(indent) << "struct " << name << " : public\n";
            writeFunc(indent + 1);
            out << output::indent(indent) << "\n" <<
                   output::indent(indent) << "{\n";
            common::writeIntIsNullFunc(out, indent + 1, nullValue);
            out << output::indent(indent) << "}";
            result = true;
            break;
        }

        log::error() << "Unkown \"presence\" token value \"" << getPresence() << "\"." << std::endl;
    } while (false);

    if (!result) {
        out << "???";
    }

    return result;
}

bool BasicType::writeSimpleFloat(std::ostream& out,
    unsigned indent,
    const std::string& fpType,
    bool isElement)
{
    std::string name;
    if (isElement) {
        name = getName() + common::elementSuffixStr();
    }
    else {
        name = getReferenceName();
    }

    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << "comms::field::FloatValue<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << fpType << ",\n";
    writeExtraOptions(out, indent + 1);

    if (isConstant()) {
        assert(!isElement);
        out << output::indent(indent + 2) << "comms::option::EmptySerialization,\n";
    }

    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";

    bool result = false;
    do {
        if (isRequired()) {
            common::writeFpValidCheckFunc(out, indent + 1);
            result = true;
            break;
        }
        
        if (isOptional()) {
            common::writeFpOptConstructor(out, indent + 1, name);
            out << '\n';
            common::writeFpIsNullFunc(out, indent + 1);
            result = true;
            break;
        }

        if (isConstant()) {
            auto constValStr = xmlText(getNode());
            assert(!constValStr.empty());
            common::writeFpOptConstructor(out, indent + 1, name, constValStr);
            out << '\n' <<
                   output::indent(indent + 1) << "/// \\brief Value validity check function.\n" <<
                   output::indent(indent + 1) << "bool valid() const\n" <<
                   output::indent(indent + 1) << "{\n";
            writeBaseDef(out, indent + 2);
            out << output::indent(indent + 2) << "auto defaultValue = static_cast<Base::ValueType>(" << constValStr << ");\n" <<
                   output::indent(indent + 2) << "return std::abs(Base::value() - defaultValue) < std::numberic_limits<Base::ValueType>::epsilon;\n" <<
                   output::indent(indent + 1) << "}\n";
            result = true;
            break;
        }

        assert(!"Invalid presence");
    } while (false);

    out << output::indent(indent) << "}";
    return result;
}

bool BasicType::writeVarLength(std::ostream& out, unsigned indent)
{
    assert(!isConstant());
    if (isString()) {
        return writeVarLengthString(out, indent);
    }

    return writeVarLengthArray(out, indent);
}


bool BasicType::writeVarLengthString(
    std::ostream& out,
    unsigned indent)
{
    out << output::indent(indent) << "struct " << getReferenceName() << " : public\n" <<
           output::indent(indent + 1) << " comms::field::String<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n";
    writeExtraOptions(out, indent + 2);
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    writeStringValidFunc(out, indent + 1);
    out << output::indent(indent) << "}";
    return true;
}

bool BasicType::writeVarLengthArray(
    std::ostream& out,
    unsigned indent)
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    if (isRawData(primType)) {
        return writeVarLengthRawDataArray(out, indent, primType);
    }

    out << output::indent(indent) << "using " << getReferenceName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << getName() << common::elementSuffixStr() << "<>,\n";
    writeExtraOptions(out, indent + 2);
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeVarLengthRawDataArray(
    std::ostream& out,
    unsigned indent,
    const std::string& primType)
{
    out << output::indent(indent) << "using " << getReferenceName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << primitiveTypeToStdInt(primType) << ",\n";
    writeExtraOptions(out, indent + 2);
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeFixedLength(
    std::ostream& out,
    unsigned indent)
{
    if (isString()) {
        return writeFixedLengthString(out, indent);
    }
    
    assert(!isConstant());
    return writeFixedLengthArray(out, indent);
}

bool BasicType::writeFixedLengthString(
    std::ostream& out,
    unsigned indent)
{
    if (!isConstString()) {
        unsigned len = getLengthProp();
        assert(1U < len);
        out << output::indent(indent) << "struct " << getReferenceName() << " : public\n" <<
               output::indent(indent + 1) << "comms::field::String<\n" <<
               output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
               output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << len << ">,\n";
        writeExtraOptions(out, indent + 2);
        out << output::indent(indent + 2) << "TOpt...\n" <<
               output::indent(indent + 1) << ">" <<
               output::indent(indent) << "{\n";
        writeStringValidFunc(out, indent + 1);
        out << output::indent(indent) << "}";
        return true;
    }

    auto text = xmlText(getNode());
    out << output::indent(indent) << "struct " << getReferenceName() << " : public \n" <<
           output::indent(indent + 1) << "comms::field::String<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << "comms::option::EmptySerialization,\n";
    writeExtraOptions(out, indent + 2);
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << getReferenceName() << "()\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << common::fieldBaseDefStr() <<
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
    unsigned indent)
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    if (isRawData(primType)) {
        return writeFixedLengthRawDataArray(out, indent, primType);
    }

    unsigned len = getLengthProp();
    assert(1U < len);

    out << output::indent(indent) << "using " << getReferenceName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << getName() << common::elementSuffixStr() << "<>,\n" <<
           output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << len << ">,\n";
    writeExtraOptions(out, indent + 2);
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeFixedLengthRawDataArray(
    std::ostream& out,
    unsigned indent,
    const std::string& primType)
{
    unsigned len = getLengthProp();
    assert(1U < len);
    out << output::indent(indent) << "using " << getReferenceName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << primitiveTypeToStdInt(primType) << ",\n" <<
           output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << len << ">,\n";
    writeExtraOptions(out, indent + 2);
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::isString() const
{
    if (!isRawData()) {
        return false;
    }

    auto semType = getSemanticType(); // by value
    boost::algorithm::to_lower(semType);
    static const std::string StringSemanticType("string");
    if (semType == StringSemanticType) {
        return true;
    }

    auto& enc = getCharacterEncoding();
    if (!enc.empty()) {
        return true;
    }

    return getPrimitiveType() == CharType;
}

bool BasicType::isConstString() const
{
    return isConstant() && isString();
}

bool BasicType::isRawData() const
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    return isRawData(primType);
}

bool BasicType::isRawData(const std::string& primType)
{
    static const std::string RawDataTypes[] = {
        CharType,
        "int8",
        "uint8"
    };
    auto iter = std::find(std::begin(RawDataTypes), std::end(RawDataTypes), primType);
    return (iter != std::end(RawDataTypes));
}

void BasicType::writeStringValidFunc(std::ostream& out, unsigned indent)
{
    auto minValue = intMinValue(CharType, std::string());
    assert (minValue.second);
    auto maxValue = intMaxValue(CharType, std::string());
    assert(maxValue.second);
    out << output::indent(indent) << "/// \\brief Value validity check function.\n" <<
           output::indent(indent) << "bool valid() const\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << common::fieldBaseDefStr() <<
           output::indent(indent + 1) << "auto& str = Base::value();\n" <<
           output::indent(indent + 1) << "for (auto ch : str) {\n" <<
           output::indent(indent + 2) << "if ((ch < " << minValue.first << ") ||\n" <<
           output::indent(indent + 2) << "    (" << maxValue.first << " < ch)) {\n" <<
           output::indent(indent + 2) << "    return false;\n" <<
           output::indent(indent + 2) << "}\n\n" <<
           output::indent(indent + 1) << "return true;\n" <<
           output::indent(indent) << "}\n";
}

void BasicType::writeExtraOptions(std::ostream& out, unsigned indent)
{
    for (auto& o : getExtraOptions()) {
        out << output::indent(indent) << o << ",\n";
    }
}

bool BasicType::hasDefaultValueInExtraOptions() const
{
    return
        std::any_of(
            getExtraOptions().begin(), getExtraOptions().end(),
            [](const std::string& o)
            {
                static const std::string OptStr("DefaultNumValue");
                return o.find(OptStr) != std::string::npos;
            });
}

} // namespace sbe2comms
