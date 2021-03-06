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
#include "DB.h"
#include "EnumType.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{

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
    auto& intType = common::primitiveTypeToStdInt(primType);
    assert(!intType.empty());
    return builtInIntNullValue(intType);
}

bool BasicType::isIntType() const
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    auto& intType = common::primitiveTypeToStdInt(primType);
    return !intType.empty();
}

bool BasicType::isFpType() const
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    auto& fpType = primitiveFloatToStd(primType);
    return !fpType.empty();
}

bool BasicType::isRawDataArray() const
{
    auto len = getLengthProp();
    if (len != 0) {
        return false;
    }

    return isRawData();
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

    do {
        bool list = (getLengthProp() != 1U);
        bool string = list && isString();
        if (string || isConstString()) {
            addExtraInclude("\"comms/field/String.h\"");
            break;
        }

        if (list) {
            addExtraInclude("\"comms/field/ArrayList.h\"");
        }

        if (isIntType()) {
            addExtraInclude("<cstdint>");
            if ((!list) || (!isRawData())) {
                addExtraInclude("\"comms/field/IntValue.h\"");
            }
        }
        else if (isFpType()) {
            addExtraInclude("<limits>");
            addExtraInclude("<cmath>");
            addExtraInclude("\"comms/field/FloatValue.h\"");
        }
    } while (false);
    return true;
}

bool BasicType::writeImpl(std::ostream& out, unsigned indent, bool commsOptionalWrapped)
{
    auto len = getLengthProp();
    if ((len != 1) && (!isString()) && (!isRawData())) {
        writeElementHeader(out, indent);
        common::writeExtraOptionsTemplParam(out, indent);
        writeSimpleType(out, indent, commsOptionalWrapped, true);
        out << ";\n\n";
    }

    writeHeader(out, indent, commsOptionalWrapped);
    common::writeExtraOptionsTemplParam(out, indent);
    bool result = false;
    do {

        if ((len == 1) && (!isConstString())) {
            result = writeSimpleType(out, indent, commsOptionalWrapped);
            break;
        }

        if (len == 0) {
            result = writeVarLength(out, indent, commsOptionalWrapped);
            break;
        }

        result = writeFixedLength(out, indent, commsOptionalWrapped);

    } while (false);
    out << ";\n\n";
    return result;
}

std::size_t BasicType::getSerializationLengthImpl() const
{
    if (isConstant()) {
        return 0U;
    }

    auto count = getLengthProp();
    auto singleLen = primitiveLength(getPrimitiveType());
    return singleLen * count;
}\

bool BasicType::hasFixedLengthImpl() const
{
    return (getLengthProp() != 0U);
}

bool BasicType::canBeExtendedAsOptionalImpl() const
{
    assert(!isConstant());
    return (getLengthProp() == 1U) && (!isConstString());
}

bool BasicType::writePluginPropertiesImpl(
        std::ostream& out,
        unsigned indent,
        const std::string& scope)
{
    auto len = getLengthProp();
    if ((len == 1) || (isRawData())) {
        return writePluginPropertiesSimple(out, indent, scope, false);
    }

    return writePluginPropertiesList(out, indent, scope);
}

bool BasicType::writeSimpleType(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped,
    bool isElement)
{
    auto& primType = getPrimitiveType();
    auto& intType = common::primitiveTypeToStdInt(primType);
    if (!intType.empty()) {
        return writeSimpleInt(out, indent, intType, commsOptionalWrapped, isElement);
    }

    auto& fpType = primitiveFloatToStd(primType);
    if (!fpType.empty()) {
        return writeSimpleFloat(out, indent, fpType, commsOptionalWrapped, isElement);
    }

    log::error() << "Unknown primitiveType \"" << primType << "\" for "
                 "field \"" << getName() << '\"' << std::endl;
    return false;
}

bool BasicType::writeSimpleBigUnsignedInt(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped,
    bool isElement,
    std::uintmax_t minVal,
    std::uintmax_t maxVal)
{
    if (maxVal < minVal) {
        log::error() << "min/max values range error for type \"" << getName() << "\"." << std::endl;
        return false;
    }

    std::uintmax_t defValue = 0;
    defValue = std::min(std::max(defValue, minVal), maxVal);
    bool constant = false;
    boost::optional<std::uintmax_t> extraValidNumber;

    auto writeFunc =
        [this, &defValue, &constant, &minVal, &maxVal, &extraValidNumber, &out](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::IntValue<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << "std::uint64_t,\n" <<
                   output::indent(ind + 1) << "TOpt...";

            writeExtraOptions(out, ind + 1);

            if (minVal != maxVal) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::ValidBigUnsignedNumValueRange<0x" <<
                            std::hex << minVal << "LL, 0x" << maxVal << std::dec << "LL>";
            }
            else {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::ValidBigUnsignedNumValue<0x" <<
                            std::hex << minVal << std::dec << "LL>";
            }

            if (extraValidNumber) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::ValidBigUnsignedNumValue<0x" <<
                            std::hex << *extraValidNumber << std::dec << "LL>";
            }

            if ((defValue != 0) && (!hasDefaultValueInExtraOptions())) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::DefaultNumValue<0x" <<
                            std::hex << defValue << std::dec << "LL>";
            }

            if (constant) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::EmptySerialization";
            }

            out << '\n' <<
                   output::indent(ind) << ">";
        };

    auto& suffix = getNameSuffix(commsOptionalWrapped, isElement);
    std::string name = common::refName(getName(), suffix);

    if (isRequired()) {
        out << output::indent(indent) << "class " << name << " : public\n";
        writeFunc(indent + 1);
        out << '\n' <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "using Base=\n";
        writeFunc(indent + 2);

        out << ";\n\n" <<
               output::indent(indent) << "public:\n";
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "}";
        return true;
    }

    if (isConstant()) {
        assert(!isElement);
        constant = true;
        auto text = nodeText();
        if (text.empty()) {
            log::error() << "Empty constant value for type \"" << getName() << "\"." << std::endl;
            return false;
        }

        try {
            defValue = std::stoull(text);
            minVal = defValue;
            maxVal = defValue;
        } catch (...) {
            log::error() << "Invalid constant value \"" << text << "\" for type \"" << getName() << "\"." << std::endl;
            return false;
        }

        out << output::indent(indent) << "class " << name << " : public\n";
        writeFunc(indent + 1);
        out << '\n' <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "using Base =\n";
        writeFunc(indent + 2);
        out << ";\n\n" <<
                output::indent(indent) << "public:\n";
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "}";
        return true;
    }

    if (isOptional()) {
        auto& nullValStr = getNullValue();
        std::uintmax_t nullValue = 0;
        do {
            if (nullValStr.empty()) {
                nullValue = common::defaultBigUnsignedNullValue();
                break;
            }

            auto val = common::intBigUnsignedMaxValue(nullValStr);
            if (!val.second) {
                log::error() << "ERROR: Bad nullValue for type \"" << getName() << "\": " << nullValStr << std::endl;
                return false;
            }

            nullValue = val.first;
        } while (false);
        defValue = nullValue;

        extraValidNumber = nullValue;
        out << output::indent(indent) << "class " << name << " : public\n";
        writeFunc(indent + 1);
        out << output::indent(indent) << "\n" <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "using Base=\n";
        writeFunc(indent + 2);
        out << ";\n\n" <<
               output::indent(indent) << "public:\n";
        common::writeIntNullCheckUpdateFuncs(out, indent + 1, common::num(nullValue), true);
        out << '\n';
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "}";
        return true;
    }

    log::error() << "Unkown \"presence\" token value \"" << getPresence() << "\"." << std::endl;
    return false;
}

bool BasicType::writeSimpleInt(
    std::ostream& out,
    unsigned indent,
    const std::string& intType,
    bool commsOptionalWrapped,
    bool isElement)
{
    auto& primType = getPrimitiveType();
    auto& minValStr = getMinValue();
    auto minVal = common::intMinValue(primType, minValStr);
    auto& maxValStr = getMaxValue();
    auto maxVal = common::intMaxValue(primType, maxValStr);

    auto checkMinMaxValErrorFunc =
        [this, &minValStr, &maxValStr](bool minValid, bool maxValid) -> bool
    {
        if (!minValid) {
            log::error() << "Invalid minValue attribute \"" << minValStr << "\" for type \"" <<
                         getName() << "\"." << std::endl;
            return false;
        }

        if (!maxValid) {
            log::error() << "Invalid maxValue attribute \"" << maxValStr << "\" for type \"" <<
                         getName() << "\"." << std::endl;
            return false;
        }

        return true;
    };

    if ((primType == common::uint64Type()) &&
        ((!minVal.second) || (!maxVal.second))) {

        auto bigMinVal = std::make_pair(static_cast<std::uintmax_t>(minVal.first), minVal.second);
        if (!bigMinVal.second) {
            bigMinVal = common::intBigUnsignedMaxValue(minValStr);
        }

        auto bigMaxVal = std::make_pair(static_cast<std::uintmax_t>(maxVal.first), maxVal.second);
        if (!bigMaxVal.second) {
            bigMaxVal = common::intBigUnsignedMaxValue(maxValStr);
        }

        if (!checkMinMaxValErrorFunc(bigMinVal.second, bigMaxVal.second)) {
            return false;
        }

        return writeSimpleBigUnsignedInt(out, indent, commsOptionalWrapped, isElement, bigMinVal.first, bigMaxVal.first);
    }

    if (!checkMinMaxValErrorFunc(minVal.second, maxVal.second)) {
        return false;
    }

    if (maxVal.first < minVal.first) {
        log::error() << "min/max values range error for type \"" << getName() << "\"." << std::endl;
        return false;
    }

    std::intmax_t defValue = 0;
    defValue = std::min(std::max(defValue, minVal.first), maxVal.first);
    bool constant = false;
    boost::optional<std::intmax_t> extraValidNumber;

    auto writeFunc =
        [this, intType, &defValue, &constant, &minVal, &maxVal, &extraValidNumber, &out](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::IntValue<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << intType << ",\n" <<
                   output::indent(ind + 1) << "TOpt...";

            writeExtraOptions(out, ind + 1);

            if (minVal.first != maxVal.first) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::ValidNumValueRange<" <<
                            common::num(minVal.first) << ", " << common::num(maxVal.first) << ">";
            }
            else {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::ValidNumValue<" <<
                            common::num(minVal.first) << ">";
            }

            if (extraValidNumber) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::ValidNumValue<" <<
                            common::num(*extraValidNumber) << ">";
            }

            if ((defValue != 0) && (!hasDefaultValueInExtraOptions())) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::DefaultNumValue<" << common::num(defValue) << ">";
            }

            if (constant) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::EmptySerialization";
            }

            out << '\n' <<
                   output::indent(ind) << ">";
        };

    auto& suffix = getNameSuffix(commsOptionalWrapped, isElement);
    std::string name = common::refName(getName(), suffix);

    if (isRequired()) {
        out << output::indent(indent) << "class " << name << " : public\n";
        writeFunc(indent + 1);
        out << '\n' <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "using Base =\n";
        writeFunc(indent + 2);
        out << ";\n\n" <<
               output::indent(indent) << "public:\n";
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "}";
        return true;
    }

    if (isConstant()) {
        assert(!isElement);
        constant = true;
        auto text = nodeText();
        if (text.empty()) {
            log::error() << "Empty constant value for type \"" << getName() << "\"." << std::endl;
            return false;
        }

        try {
            defValue = std::stoll(text);
            minVal.first = defValue;
            maxVal.first = defValue;
        } catch (...) {
            log::error() << "Invalid constant value \"" << text << "\" for type \"" << getName() << "\"." << std::endl;
            return false;
        }

        out << output::indent(indent) << "class " << name << " : public\n";
        writeFunc(indent + 1);
        out << '\n' <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "using Base=\n";
        writeFunc(indent + 2);
        out << ";\n\n" <<
               output::indent(indent) << "public:\n";
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "}";

        return true;
    }

    if (isOptional()) {
        auto& nullValStr = getNullValue();
        std::intmax_t nullValue = 0;
        do {
            if (nullValStr.empty()) {
                nullValue = builtInIntNullValue(intType);
                break;
            }

            if ((intType == common::charType()) && (nullValStr.size() == 1U)) {
                nullValue = static_cast<std::intmax_t>(nullValStr[0]);
                break;
            }

            auto convertResult = stringToInt(nullValStr);
            if (!convertResult.second) {
                log::error() << "ERROR: Bad nullValue for type \"" << getName() << "\": " << nullValStr << std::endl;
                return false;
            }

            nullValue = convertResult.first;
        } while (false);
        defValue = nullValue;

        extraValidNumber = nullValue;
        out << output::indent(indent) << "class " << name << " : public\n";
        writeFunc(indent + 1);
        out << output::indent(indent) << "\n" <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "using Base=\n";
        writeFunc(indent + 2);
        out << ";\n\n" <<
               output::indent(indent) << "public:\n";
        common::writeIntNullCheckUpdateFuncs(out, indent + 1, common::num(nullValue), true);
        out << '\n';
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "}";
        return true;
    }

    log::error() << "Unkown \"presence\" token value \"" << getPresence() << "\"." << std::endl;
    return false;
}

bool BasicType::writeSimpleFloat(
    std::ostream& out,
    unsigned indent,
    const std::string& fpType,
    bool commsOptionalWrapped,
    bool isElement)
{
    auto& suffix = getNameSuffix(commsOptionalWrapped, isElement);
    std::string name = common::refName(getName(), suffix);

    auto writeClassDefFunc =
        [this, &out, &fpType, isElement](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::FloatValue<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << fpType << ",\n" <<
                   output::indent(ind + 1) << "TOpt...";
            writeExtraOptions(out, ind);
            if (isConstant()) {
                assert(!isElement);
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::EmptySerialization";
            }

            out << '\n' <<
                   output::indent(ind) << ">";
        };

    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);

    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base=\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n";

    bool result = false;
    do {
        if (isRequired()) {
            common::writeFpValidCheckFunc(out, indent + 1, false, true);
            out << '\n';
            common::writeDefaultSetVersionFunc(out, indent + 1);
            result = true;
            break;
        }
        
        if (isOptional()) {
            common::writeFpOptConstructor(out, indent + 1, name, common::emptyString(), true);
            out << '\n';
            common::writeFpNullCheckUpdateFuncs(out, indent + 1, true);
            out << '\n';
            common::writeDefaultSetVersionFunc(out, indent + 1);
            result = true;
            break;
        }

        if (isConstant()) {
            auto constValStr = xmlText(getNode());
            assert(!constValStr.empty());
            common::writeFpOptConstructor(out, indent + 1, name, constValStr, true);
            out << '\n' <<
                   output::indent(indent + 1) << "/// \\brief Value validity check function.\n" <<
                   output::indent(indent + 1) << "bool valid() const\n" <<
                   output::indent(indent + 1) << "{\n" <<
                   output::indent(indent + 2) << "auto defaultValue = static_cast<typename Base::ValueType>(" << constValStr << ");\n" <<
                   output::indent(indent + 2) << "return std::abs(Base::value() - defaultValue) < std::numberic_limits<typename Base::ValueType>::epsilon;\n" <<
                   output::indent(indent + 1) << "}\n\n";
            common::writeDefaultSetVersionFunc(out, indent + 1);
            result = true;
            break;
        }

        assert(!"Invalid presence");
    } while (false);

    out << output::indent(indent) << "}";
    return result;
}

bool BasicType::writeVarLength(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    assert(!isConstant());
    if (isString()) {
        return writeVarLengthString(out, indent, commsOptionalWrapped);
    }

    return writeVarLengthArray(out, indent, commsOptionalWrapped);
}

bool BasicType::writeVarLengthString(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    std::string name = common::refName(getName(), suffix);

    auto writeClassDefFunc =
        [this, &out](unsigned ind)
        {
            out << output::indent(ind) << " comms::field::String<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << "TOpt...";
            writeExtraOptions(out, ind + 1);
            out << '\n' <<
                   output::indent(ind) << ">";
        };

    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 2) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:";
    writeStringValidFunc(out, indent + 1);
    out << '\n';
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "}";
    return true;
}

bool BasicType::writeVarLengthArray(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    if (isRawData(primType)) {
        return writeVarLengthRawDataArray(out, indent, primType, commsOptionalWrapped);
    }

    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    std::string name = common::refName(getName(), suffix);

    auto writeClassDefFunc =
            [this, &out](unsigned ind)
            {
                out << output::indent(ind) << "comms::field::ArrayList<\n" <<
                       output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                       output::indent(ind + 1) << getName() << common::elementSuffixStr() << "<>,\n" <<
                       output::indent(ind + 1) << "TOpt...";
                writeExtraOptions(out, ind + 1);
                out << '\n' <<
                       output::indent(ind) << ">";
            };

    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "}";
    return true;
}

bool BasicType::writeVarLengthRawDataArray(
    std::ostream& out,
    unsigned indent,
    const std::string& primType,
    bool commsOptionalWrapped)
{
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    std::string name = common::refName(getName(), suffix);

    auto writeClassDefFunc =
        [this, &out, &primType](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::ArrayList<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << common::primitiveTypeToStdInt(primType) << ",\n" <<
                   output::indent(ind + 1) << "TOpt...";
            writeExtraOptions(out, ind + 1);
            out << '\n' <<
                   output::indent(ind) << ">";
        };

    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "}";
    return true;
}

bool BasicType::writeFixedLength(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    if (isString()) {
        return writeFixedLengthString(out, indent, commsOptionalWrapped);
    }
    
    assert(!isConstant());
    return writeFixedLengthArray(out, indent, commsOptionalWrapped);
}

bool BasicType::writeFixedLengthString(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    std::string name = common::refName(getName(), suffix);

    if (!isConstString()) {
        unsigned len = getLengthProp();
        assert(1U < len);

        auto writeClassDefFunc =
            [this, &out, len](unsigned ind)
            {
                out << output::indent(ind) << "comms::field::String<\n" <<
                       output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                       output::indent(ind + 1) << "comms::option::SequenceFixedSize<" << len << ">,\n" <<
                       output::indent(ind + 1) << "TOpt...";
                writeExtraOptions(out, ind + 1);
                out << '\n' <<
                       output::indent(ind) << ">\n";
            };

        out << output::indent(indent) << "class " << name << " : public\n";
        writeClassDefFunc(indent + 1);
        out << '\n' <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "using Base =\n";
        writeClassDefFunc(indent + 2);
        out << ";\n\n" <<
               output::indent(indent) << "public:\n";
        writeStringValidFunc(out, indent + 1);
        out << '\n';
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "}";
        return true;
    }

    auto writeClassDefFunc =
        [this, &out](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::String<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << "TOpt...";
            writeExtraOptions(out, ind + 1);
            out << ",\n" <<
                   output::indent(ind + 1) << "comms::option::EmptySerialization\n" <<
                   output::indent(ind) << ">";
        };

    auto text = xmlText(getNode());
    out << output::indent(indent) << "class " << name << " : public \n";
    writeClassDefFunc(indent + 1);
    out << "\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n" <<
           output::indent(indent + 1) << "/// \\brief Default constructor\n" <<
           output::indent(indent + 1) << getReferenceName() << "()\n" <<
           output::indent(indent + 1) << "{\n" <<
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
           output::indent(indent + 2) << "};\n\n" <<
           output::indent(indent + 2) << "Base::value() = Chars;\n" <<
           output::indent(indent + 1) << "}\n\n";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "}";
    
    return true;
}

bool BasicType::writeFixedLengthArray(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped)
{
    auto& primType = getPrimitiveType();
    assert(!primType.empty());
    if (isRawData(primType)) {
        return writeFixedLengthRawDataArray(out, indent, primType, commsOptionalWrapped);
    }

    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    std::string name = common::refName(getName(), suffix);

    unsigned len = getLengthProp();
    assert(1U < len);

    auto writeClassDefFunc =
        [this, &out, len](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::ArrayList<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << getName() << common::elementSuffixStr() << "<>,\n" <<
                   output::indent(ind + 1) << "TOpt...,\n" <<
                   output::indent(ind + 1) << "comms::option::SequenceFixedSize<" << len << ">";
            writeExtraOptions(out, ind + 1);
            out << '\n' <<
                   output::indent(ind) << ">";

        };

    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "}";
    return true;
}

bool BasicType::writeFixedLengthRawDataArray(
    std::ostream& out,
    unsigned indent,
    const std::string& primType,
    bool commsOptionalWrapped)
{
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    std::string name = common::refName(getName(), suffix);

    unsigned len = getLengthProp();
    assert(1U < len);

    auto writeClassDefFunc =
        [this, &out, &primType, len](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::ArrayList<\n" <<
                   output::indent(ind + 1) << getFieldBaseString() << ",\n" <<
                   output::indent(ind + 1) << common::primitiveTypeToStdInt(primType) << ",\n" <<
                   output::indent(ind + 1) << "TOpt...,\n" <<
                   output::indent(ind + 1) << "comms::option::SequenceFixedSize<" << len << ">";
            writeExtraOptions(out, ind + 1);
            out << '\n' <<
                   output::indent(ind) << ">";
        };
    out << output::indent(indent) << "class " << name << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base =\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "}";
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

    return getPrimitiveType() == common::charType();
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

bool BasicType::isRawData(const std::string& primType) const
{
    if (isOptional()) {
        return false;
    }

    static const std::string RawDataTypes[] = {
        common::charType(),
        "int8",
        "uint8"
    };
    auto iter = std::find(std::begin(RawDataTypes), std::end(RawDataTypes), primType);
    return (iter != std::end(RawDataTypes));
}

void BasicType::writeStringValidFunc(
    std::ostream& out,
    unsigned indent)
{
    auto minValue = common::intMinValue(common::charType(), std::string());
    assert (minValue.second);
    auto maxValue = common::intMaxValue(common::charType(), std::string());
    assert(maxValue.second);
    out << output::indent(indent) << "/// \\brief Value validity check function.\n" <<
           output::indent(indent) << "bool valid() const\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "if (!Base::valid()) {\n" <<
           output::indent(indent + 2) << "return false;\n" <<
           output::indent(indent + 1) << "}\n\n" <<
           output::indent(indent + 1) << "auto& str = Base::value();\n" <<
           output::indent(indent + 1) << "for (auto ch : str) {\n" <<
           output::indent(indent + 2) << "if ((ch < " << minValue.first << ") ||\n" <<
           output::indent(indent + 2) << "    (" << maxValue.first << " < ch)) {\n" <<
           output::indent(indent + 2) << "    return false;\n" <<
           output::indent(indent + 2) << "}\n\n" <<
           output::indent(indent + 1) << "}\n" <<
           output::indent(indent + 1) << "return true;\n" <<
           output::indent(indent) << "}\n";
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

bool BasicType::writePluginPropertiesSimple(
    std::ostream& out,
    unsigned indent,
    const std::string& scope,
    bool isElement,
    int index)
{
    std::string fieldType;
    std::string props;
    common::scopeToPropertyDefNames(scope, getName(), (!isElement) && isCommsOptionalWrapped(), &fieldType, &props);

    if (isElement) {
        fieldType += common::elementSuffixStr();
        props += common::elementSuffixStr();

        if (0 <= index) {
            auto indexStr = std::to_string(index);
            fieldType += indexStr;
            props += indexStr;
        }
    }

    bool commsOptionalWrapped = isCommsOptionalWrapped();
    auto& suffix = getNameSuffix(commsOptionalWrapped, isElement);
    auto name = common::refName(getName(), suffix);

    std::string dispOffsetStr;
    for (auto& o : getExtraOptions()) {
        static const std::string OptPrefix("comms::option::NumValueSerOffset<");
        if (!ba::starts_with(o, OptPrefix)) {
            continue;
        }

        auto findResult = ba::find_last(o, ">");
        assert(findResult);
        dispOffsetStr = ".displayOffset(" + std::string(o.begin() + OptPrefix.size(), findResult.begin()) + ")";
    }

    std::string fieldName;
    do {
        if (isElement && (index < 0)) {
            fieldName = "\"elem\"";
            break;
        }

        if (isElement) {
            fieldName = '\"' + std::to_string(index) + '\"';
            break;
        }

        if (scope.empty()) {
            fieldName = common::fieldNameParamNameStr();
            break;
        }

        fieldName = '\"' + getName() + '\"';
    } while (false);

    out << output::indent(indent) << "using " << fieldType << " = " <<
           common::scopeFor(getDb().getProtocolNamespace(), common::fieldNamespaceStr() + scope + name) <<
           "<>;\n" <<
           output::indent(indent) << "auto " << props << " = \n" <<
           output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType << ">()\n" <<
           output::indent(indent + 2) << ".name(" << fieldName << ")";

    if (isConstant()) {
        out << '\n' <<
               output::indent(indent + 2) << ".serialisedHidden()\n" <<
               output::indent(indent + 2) << ".readOnly()\n";
    }

    if (!dispOffsetStr.empty()) {
        out << '\n' <<
               output::indent(indent + 2) << dispOffsetStr;
    }

    out << ";\n\n";

    writeSerialisedHiddenCheck(out, indent, props);

    if (scope.empty() && (!commsOptionalWrapped) && (!isElement)) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }
    return true;
}

bool BasicType::writePluginPropertiesList(
    std::ostream& out,
    unsigned indent,
    const std::string& scope)
{
    std::string fieldType;
    std::string props;
    scopeToPropertyDefNames(scope, &fieldType, &props);

    auto len = getLengthProp();
    assert(len != 1U);
    if (len == 0) {
        writePluginPropertiesSimple(out, indent, scope, true);
    }

    for (auto idx = 0; idx < static_cast<int>(len); ++idx) {
        writePluginPropertiesSimple(out, indent, scope, true, idx);
    }

    bool commsOptionalWrapped = isCommsOptionalWrapped();
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);

    out << output::indent(indent) << "using " << fieldType << " = " <<
           common::scopeFor(getDb().getProtocolNamespace(), common::fieldNamespaceStr() + scope + name) <<
           "<>;\n" <<
           output::indent(indent) << "auto " << props << " = \n" <<
           output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType << ">()\n" <<
           output::indent(indent + 2) << ".serialisedHidden()\n" <<
           output::indent(indent + 2) << ".name(";
    if (scope.empty()) {
        out  << common::fieldNameParamNameStr();
    }
    else {
        out << '\"' << getName() << '\"';
    }
    out << ")";

    std::string elemProps;
    common::scopeToPropertyDefNames(scope, getName(), false, nullptr, &elemProps);
    elemProps += common::elementSuffixStr();


    if (len == 0) {
        out << '\n' <<
               output::indent(indent + 2) << ".add(" << elemProps << ".asMap())";
    }

    for (auto idx = 0; idx < static_cast<int>(len); ++idx) {
        out << '\n' <<
               output::indent(indent + 2) << ".add(" << elemProps << std::to_string(idx) << ".asMap())";
    }

    out << ";\n\n";

    if (scope.empty() && (!commsOptionalWrapped)) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }
    return true;
}

} // namespace sbe2comms
