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

#include "prop.h"
#include "get.h"
#include "output.h"

namespace sbe2comms
{

namespace
{

const std::string& primitiveIntToStd(const std::string& type)
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
        return get::emptyString();
    }

    return iter->second;
}

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

template <typename T>
std::string toSignedMinInt()
{
    return std::to_string(std::numeric_limits<T>::min() + 1) + "LL";
}

std::pair<std::intmax_t, bool> intMinValue(const std::string& type, const std::string& value)
{
    if (value.empty()) {
        static const std::map<std::string, std::intmax_t> Map = {
            std::make_pair("char", std::numeric_limits<char>::min() + 1),
            std::make_pair("int8", std::numeric_limits<std::int8_t>::min() + 1),
            std::make_pair("uint8", 0),
            std::make_pair("int16", std::numeric_limits<std::int16_t>::min() + 1),
            std::make_pair("uint16", 0),
            std::make_pair("int32", std::numeric_limits<std::int32_t>::min() + 1),
            std::make_pair("uint32", 0),
            std::make_pair("int64", std::numeric_limits<std::int64_t>::min() + 1),
            std::make_pair("uint64", 0)
        };

        auto iter = Map.find(type);
        assert(iter != Map.end());
        return std::make_pair(iter->second, true);
    }

    try {
        return std::make_pair(std::stoll(value), true);
    } catch(...) {
        return std::make_pair(std::intmax_t(0), false);
    }
}

std::pair<std::intmax_t, bool> intMaxValue(const std::string& type, const std::string& value)
{
    if (value.empty()) {
        static const std::map<std::string, std::intmax_t> Map = {
            std::make_pair("char", std::numeric_limits<char>::max()),
            std::make_pair("int8", std::numeric_limits<std::int8_t>::max()),
            std::make_pair("uint8", std::numeric_limits<std::uint8_t>::max() - 1),
            std::make_pair("int16", std::numeric_limits<std::int16_t>::max()),
            std::make_pair("uint16", std::numeric_limits<std::uint16_t>::max() - 1),
            std::make_pair("int32", std::numeric_limits<std::int32_t>::max()),
            std::make_pair("uint32", std::numeric_limits<std::uint32_t>::max() - 1),
            std::make_pair("int64", std::numeric_limits<std::int64_t>::max()),
            std::make_pair("uint64", std::numeric_limits<std::uint64_t>::max() - 1)
        };

        auto iter = Map.find(type);
        assert(iter != Map.end());
        return std::make_pair(iter->second, true);
    }

    try {
        return std::make_pair(std::stoll(value), true);
    } catch(...) {
        return std::make_pair(std::intmax_t(0), false);
    }
}

std::string toString(std::intmax_t val) {
    return std::to_string(val) + "LL";
}

} // namespace

bool BasicType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    writeBrief(out, db, indent);

    auto& p = props(db);

    out << output::indent(indent) << "using " << prop::name(p) << " = ";

    auto& primType = prop::primitiveType(p);
    if (primType.empty()) {
        std::cerr << "ERROR: Primitive type was not provided for type \"" << prop::name(p) << "\"" << std::endl;
        out << get::unknownValueString() << "\n\n";
        return false;
    }

    unsigned length = prop::length(p);
    if (length == 1) {
        return writeSimpleType(out, db, indent, primType);
    }

    if (length == 0) {
        // TODO: ???
        assert(!"NYI");
        return false;
    }

    return writeFixedLength(out, db, indent, primType);
}

bool BasicType::writeSimpleType(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    auto& intType = primitiveIntToStd(primType);
    if (!intType.empty()) {
        return writeSimpleInt(out, db, indent, intType);
    }

    auto& fpType = primitiveFloatToStd(primType);
    if (!fpType.empty()) {
        return writeSimpleFloat(out, db, indent, fpType);
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
    const std::string& intType)
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

        if (prop::isRequired(p)) {
            std::intmax_t defValue = 0;
            defValue = std::min(std::max(defValue, minVal.first), maxVal.first);
            out << "\n" <<
                   output::indent(indent + 1) << "comms::field::IntValue<\n" <<
                   output::indent(indent + 2) << "FieldBase,\n" <<
                   output::indent(indent + 2) << intType << ",\n" <<
                   output::indent(indent + 2) << "comms::option::ValidNumValueRange<" <<
                        toString(minVal.first) << ", " << toString(maxVal.first) << ">";
            if (defValue != 0) {
                out << ",\n" <<
                       output::indent(indent + 2) << "comms::option::DefaultNumValue<" << toString(defValue) << ">";
            }
            out << "\n" << output::indent(indent + 1) << ">";
            result = true;
            break;
        }

        // TODO constant or optional
    } while (false);

    if (!result) {
        out << get::unknownValueString();
    }

    out << ";\n\n";
    return result;

    static_cast<void>(indent);
    static_cast<void>(intType);
}

bool BasicType::writeSimpleFloat(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& fpType)
{
    out << get::unknownValueString() << "\n\n";
    static_cast<void>(db);
    static_cast<void>(indent);
    static_cast<void>(fpType);
    return true;
}

bool BasicType::writeFixedLength(
    std::ostream& out,
    DB& db,
    unsigned indent,
    const std::string& primType)
{
    out << get::unknownValueString() << "\n\n";
    static_cast<void>(db);
    static_cast<void>(indent);
    static_cast<void>(primType);
    return true;
}

bool BasicType::hasMinMaxValues(DB& db)
{
    auto& p = props(db);
    return prop::hasMinValue(p) || prop::hasMaxValue(p);
}

} // namespace sbe2comms
