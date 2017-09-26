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
#include "get.h"
#include "output.h"

namespace sbe2comms
{

namespace
{

const std::string CharType("char");
const std::string ValidatorSuffix("Validator");
const std::string InitSuffix("Initializer");

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

const std::string& BasicType::getPrimitiveType() const
{
    return prop::primitiveType(getProps());
}

BasicType::Kind BasicType::kindImpl() const
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

bool BasicType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    static_cast<void>(db);
    if (!writeSimpleInitializer(out, indent)) {
        return false;
    }

    if (!writeSimpleValidator(out, indent)) {
        return false;
    }

    writeBrief(out, indent);
    writeOptions(out, indent);

    bool result = false;
    do {
        auto& primType = getPrimitiveType();
        assert(!primType.empty());

        unsigned length = getLengthProp();
        if ((length == 1) && (!isConstString())) {
            result = writeSimpleType(out, indent);
            break;
        }

        if (length == 0) {
            result = writeVarLength(out, indent, primType);
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
    unsigned indent,
    bool embedded)
{
    auto& primType = getPrimitiveType();
    auto& intType = primitiveTypeToStdInt(primType);
    if (!intType.empty()) {
        return writeSimpleInt(out, indent, intType, embedded);
    }

    auto& fpType = primitiveFloatToStd(primType);
    if (!fpType.empty()) {
        return writeSimpleFloat(out, indent, fpType, embedded);
    }

    log::error() << "Unknown primitiveType \"" << primType << "\" for "
                 "field \"" << getName() << '\"' << std::endl;
    return false;
}

bool BasicType::writeSimpleInt(
    std::ostream& out,
    unsigned indent,
    const std::string& intType,
    bool embedded)
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
            [this, embedded, intType, &defValue, &constant, &minVal, &maxVal, &extraValidNumber, &out](unsigned ind)
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

                writeFailOnInvalid(out, ind + 1);

                if (!embedded) {
                    out << ",\n" <<
                           output::indent(ind + 1) << "TOpt...";
                }

                out << "\n" << output::indent(ind) << ">";
            };

        if (isRequired()) {
            if (!embedded) {
                out << output::indent(indent) << "using " << getName() << " = \n";
            }
            writeFunc(indent + 1);
            result = true;
            break;
        }

        if (isConstant()) {
            assert(!embedded);
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

            out << output::indent(indent) << "using " << getName() << " = \n";
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
            if (!embedded) {
                out << output::indent(indent) << "struct " << getName() << " : public\n";
            }
            writeFunc(indent + 1);
            if (!embedded) {
                out << output::indent(indent) << "\n" <<
                       output::indent(indent) << "{\n" <<
                       output::indent(indent + 1) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
                       output::indent(indent + 1) << "bool isNull() const\n" <<
                       output::indent(indent + 2) << "using Base = typename std::decay<decltype(comms::field::toFieldBase(*this))>::type;\n" <<
                       output::indent(indent + 2) << "return Base::value() == static_cast<Base::ValueType>(" << nullValue << ");\n" <<
                       output::indent(indent + 1) << "}\n" <<
                       output::indent(indent) << "}";
            }
            result = true;
            break;
        }

        log::error() << "Unkown \"presence\" token value \"" << getPresence() << "\"." << std::endl;
    } while (false);

    if (!result) {
        out << get::unknownValueString();
    }

    return result;
}

bool BasicType::writeSimpleFloat(
    std::ostream& out,
    unsigned indent,
    const std::string& fpType,
    bool embedded)
{
    auto& name = getName();
    if (!embedded) {
        out << output::indent(indent) << "struct " << name << " : public\n";
    }

    out << output::indent(indent + 1) << "comms::field::FloatValue<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << fpType;

    writeFailOnInvalid(out, indent + 2);

    if (isConstant()) {
        assert(!embedded);
        out << ",\n" <<
               output::indent(indent + 2) << "comms::option::EmptySerialization";
    }

    if (embedded) {
        out << ",\n";
        if (!isOptional()) {
            out << output::indent(indent + 2) << "comms::option::ContentsValidator<" << name << ValidatorSuffix << ">\n";
        }
        else {
            out << output::indent(indent + 2) << "comms::option::DefaultValueInitialiser<" << name << InitSuffix << ">\n";
        }

        out << output::indent(indent + 1) << ">";
        return true;
    }

    out << ",\n" <<
           output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";

    bool result = false;
    do {
        auto writeConstractorFunc =
            [this, &out, &name](unsigned ind, const std::string& val)
            {
                out << output::indent(ind) << "/// \\brief Default constructor.\n" <<
                       output::indent(ind) << name << "::" << name << "()\n" <<
                       output::indent(ind) << "{\n";
                writeBaseDef(out, ind + 1);
                out << output::indent(ind + 1) << "Base::value() = " << val << ";\n" <<
                       output::indent(ind) << "}\n";
            };


        if (isRequired()) {
            out << output::indent(indent + 1) << "/// \\brief Value validity check function.\n" <<
                   output::indent(indent + 1) << "bool valid() const\n" <<
                   output::indent(indent + 1) << "{\n";
            writeBaseDef(out, indent + 2);
            out << output::indent(indent + 2) << "return Base::valid() && (!std::isnan(Base::value()));\n" <<
                   output::indent(indent + 1) << "}\n";
            result = true;
            break;
        }
        
        if (isOptional()) {
            writeConstractorFunc(indent + 1, "std::numeric_limits<Base::ValueType>::quiet_NaN()");
            out << '\n' <<
                   output::indent(indent + 1) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
                   output::indent(indent + 1) << "bool isNull() const\n" <<
                   output::indent(indent + 1) << "{\n";
            writeBaseDef(out, indent + 2);
            out << output::indent(indent + 2) << "return std::isnan(Base::value());\n" <<
                   output::indent(indent + 1) << "}\n";
            result = true;
            break;
        }

        if (isConstant()) {
            auto constValStr = xmlText(getNode());
            assert(!constValStr.empty());
            writeConstractorFunc(indent + 1, "static_cast<Base::ValueType>(" + constValStr + ")");
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

bool BasicType::writeSimpleValidator(
    std::ostream& out,
    unsigned indent)
{
    auto& primType = getPrimitiveType();
    auto& fpType = primitiveFloatToStd(primType);
    if (!fpType.empty()) {
        return writeSimpleFloatValidator(out, indent);
    }

    return true;
}

bool BasicType::writeSimpleFloatValidator(
    std::ostream& out,
    unsigned indent)
{
    if (!isRequired()) {
        return true; // nothing to do
    }

    auto len = getLengthProp();
    if (len == 1U) {
        return true; // nothing to do for non-embedded definitions
    }

    out << output::indent(indent) << "/// \\brief Custom validator for \\ref " << getName() << " field.\n" <<
           output::indent(indent) << "struct " << getName() << ValidatorSuffix << "\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Checks the value is valid.\n" <<
           output::indent(indent + 1) << "/// \\return \\b true if and only if the value is not \\b NaN.\n" <<
           output::indent(indent + 1) << "template <typename TField>\n" <<
           output::indent(indent + 1) << "bool operator()(const TField& field) const\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "return !std::isnan(field.value());\n" <<
           output::indent(indent + 1) << "}\n" <<
           output::indent(indent) << "};\n\n";
    return true;
}

bool BasicType::writeSimpleInitializer(
    std::ostream& out,
    unsigned indent)
{
    auto& primType = getPrimitiveType();
    auto& fpType = primitiveFloatToStd(primType);
    if (!fpType.empty()) {
        return writeSimpleFloatInitializer(out, indent);
    }

    return true;
}

bool BasicType::writeSimpleFloatInitializer(
    std::ostream& out,
    unsigned indent)
{
    if (!isOptional()) {
        return true; // nothing to do
    }

    auto len = getLengthProp();
    if (len == 1U) {
        return true; // nothing to do for not embedded definitions
    }

    out << output::indent(indent) << "/// \\brief Custom inititializer for \\ref " << getName() << " field.\n" <<
           output::indent(indent) << "struct " << getName() << InitSuffix << "\n" <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "/// \\brief Initializes inner value to \\b NaN.\n" <<
           output::indent(indent + 1) << "template <typename TField>\n" <<
           output::indent(indent + 1) << "void operator()(TField& field) const\n" <<
           output::indent(indent + 1) << "{\n" <<
           output::indent(indent + 2) << "using Field = typename std::decay<decltype(field)>::type;\n" <<
           output::indent(indent + 2) << "field.value() = std::numeric_limits<typename Field::ValueType>::quiet_NaN();\n" <<
           output::indent(indent + 1) << "}\n" <<
           output::indent(indent) << "};\n\n";
    return true;
}

bool BasicType::writeVarLength(
    std::ostream& out,
    unsigned indent,
    const std::string& primType)
{
    assert(!isConstant());
    if (isString()) {
        return writeVarLengthString(out, indent);
    }

    return writeVarLengthArray(out, indent, primType);
}


bool BasicType::writeVarLengthString(
    std::ostream& out,
    unsigned indent)
{
    out << output::indent(indent) << "using " << getName() << " = comms::field::String<FieldBase, TOpt...>";
    return true;
}

bool BasicType::writeVarLengthArray(
    std::ostream& out,
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
        return writeVarLengthRawDataArray(out, indent, primType);
    }

    out << output::indent(indent) << "using " << getName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << "FieldBase,\n";
    writeSimpleType(out, indent + 1, true);
    out << "\n" <<
           output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeVarLengthRawDataArray(
    std::ostream& out,
    unsigned indent,
    const std::string& primType)
{
    out << output::indent(indent) << "using " << getName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << primitiveTypeToStdInt(primType) << ",\n" <<
           output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">";
    return true;
}

bool BasicType::writeFixedLength(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    if (isString()) {
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
    if (!isConstString()) {
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
    writeSimpleType(out, indent + 1, true);
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

bool BasicType::isString()
{
    static const std::string StringTypes[] = {
        CharType,
        "int8",
        "uint8"
    };

    auto& primType = getPrimitiveType();
    auto iter = std::find(std::begin(StringTypes), std::end(StringTypes), primType);
    if (iter == std::end(StringTypes)) {
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

    return primType == CharType;
}

bool BasicType::isConstString()
{
    return isConstant() && isString();
}


} // namespace sbe2comms
