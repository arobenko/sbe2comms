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

#include "EnumType.h"

#include <iostream>
#include <set>
#include <cassert>
#include <cmath>

#include <boost/optional.hpp>

#include "common.h"
#include "output.h"
#include "log.h"
#include "BasicType.h"
#include "DB.h"

namespace sbe2comms
{

namespace
{

const std::size_t MaxRangesCount = 10;

} // namespace


bool EnumType::hasValue(const std::string& name) const
{
    return findValue(name) != m_values.end();
}

std::intmax_t EnumType::getNumericValue(const std::string& name) const
{
    auto iter = findValue(name);
    if (iter == m_values.end()) {
        assert(!"Mustn't happen");
        return 0;
    }
    return iter->first;
}

std::intmax_t EnumType::getDefultNullValue() const
{
    auto& underlying = getUnderlyingType();
    assert(!underlying.empty());
    return builtInIntNullValue(underlying);
}

EnumType::Kind EnumType::getKindImpl() const
{
    return Kind::Enum;
}

bool EnumType::parseImpl()
{
    if (isConstant()) {
        log::error() << "Constant enum types are not supported. See definition of \"" << getName() << "\"." << std::endl;
        return false;
    }

    if (isOptional()) {
        auto& nullValue = getNullValue();
        if (nullValue.empty()) {
            log::error() << "Optional enum \"" << getName() << "\" doesn't specify nullValue." << std::endl;
            return false;
        }
    }

    auto& underlying = getUnderlyingType();
    if (underlying.empty()) {
        return false;
    }

    if (!readValues()) {
        return false;
    }

    addExtraInclude("\"comms/field/EnumValue.h\"");
    auto ranges = getValidRanges();
    if (MaxRangesCount < ranges.size()) {
        addExtraInclude("<algorithm>");
    }

    if (getLengthProp() != 1U) {
        log::warning() << "Ignoring \"length\" property of \"" << getName() << "\" type to match sbe-tool." << std::endl;
    }

    return true;
}

bool EnumType::writeImpl(std::ostream& out, unsigned indent, bool commsOptionalWrapped)
{
    auto count = getAdjustedLengthProp();
    if (count != 1U) {
        writeSingle(out, indent, commsOptionalWrapped, true);
    }

    if (count == 1U) {
        writeSingle(out, indent, commsOptionalWrapped);
        return true;
    }

    writeList(out, indent, count, commsOptionalWrapped);
    return true;
}

std::size_t EnumType::getSerializationLengthImpl() const
{
    auto& encType = getEncodingType();
    assert(!encType.empty());

    auto& types = getDb().getTypes();
    auto iter = types.find(encType);
    if (iter == types.end()) {
        auto len = primitiveLength(encType);
        if (len == 0) {
            log::error() << "Unknown encoding type \"" << encType << "\" for enum \"" << getName() << "\"" << std::endl;
        }

        return len;
    }

    assert(iter->second);
    auto k = iter->second->getKind();
    if (k != Kind::Basic) {
        log::error() << "Only basic type can be used as encodingType for enum \"" << getName() << "\"" << std::endl;
        return 0U;
    }

    return iter->second->getSerializationLength();
}

bool EnumType::hasFixedLengthImpl() const
{
    return getAdjustedLengthProp() != 0U;
}

bool EnumType::canBeExtendedAsOptionalImpl() const
{
    assert(!isConstant());
    return getAdjustedLengthProp() == 1U;
}

bool EnumType::writePluginPropertiesImpl(
    std::ostream& out,
    unsigned indent,
    const std::string& scope)
{
    std::string fieldType;
    std::string props;
    scopeToPropertyDefNames(scope, &fieldType, &props);
    auto nameStr = common::fieldNameParamNameStr();
    if (!scope.empty()) {
        nameStr = '\"' + getName() + '\"';
    }

    bool commsOptionalWrapped = isCommsOptionalWrapped();
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);

    out << output::indent(indent) << "using " << fieldType << " = " <<
           common::scopeFor(getDb().getProtocolNamespace(), common::fieldNamespaceStr() + scope + name) <<
           "<>;\n" <<
           output::indent(indent) << "auto " << props << " = \n" <<
           output::indent(indent + 1) << "comms_champion::property::field::ForField<" << fieldType << ">()\n" <<
           output::indent(indent + 2) << ".name(" << nameStr << ")";
    for (auto& v : m_values) {
        out << '\n' <<
               output::indent(indent + 2) << ".add(\"" << v.second << "\", " << v.first << ")";
    }
    out << ";\n\n";

    writeSerialisedHiddenCheck(out, indent, props);

    if (scope.empty() && (!commsOptionalWrapped)) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }

    return true;
}

void EnumType::writeSingle(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped,
    bool isElement)
{
    auto& underlying = getUnderlyingType();
    assert(!underlying.empty());

    std::string enumName;
    std::string valuePrefix;
    do {
        if (m_msgId) {
            enumName = common::scopeFor(getDb().getProtocolNamespace(), common::msgIdEnumName());
            valuePrefix = enumName + '_';
            break;
        }

        enumName = getName() + common::enumValSuffixStr();
        valuePrefix = enumName + "::";
        out << output::indent(indent) << "/// \\brief Enumeration for \\ref " << getReferenceName() << " field.\n" <<
               output::indent(indent) << "enum class " << enumName << " : " << underlying << '\n' <<
               output::indent(indent) << "{\n";
        for (auto& v : m_values) {
            out << output::indent(indent + 1) << v.second << " = static_cast<" <<
                   underlying << ">(" << v.first << "), ///< ";
            auto descIter = m_desc.find(v.second);
            if (descIter == m_desc.end()) {
                out << "\\b " << v.second << " value.\n";
                continue;
            }

            out << descIter->second << "\n";
        }
        out << output::indent(indent) << "};\n\n";
    } while (false);

    if (isElement) {
        writeElementHeader(out, indent);
    }
    else {
        writeHeader(out, indent, commsOptionalWrapped, true);
    }
    out << output::indent(indent) << "/// \\see \\ref " << enumName << '\n';

    auto& suffix = getNameSuffix(commsOptionalWrapped, isElement);
    auto name = common::refName(getName(), suffix);
    common::writeExtraOptionsTemplParam(out, indent);

    auto ranges = getValidRanges();
    bool tooManyRanges = MaxRangesCount < ranges.size();
    bool asType = (!isOptional()) && (!tooManyRanges);
    assert(!ranges.empty());
    std::intmax_t defValue = 0;
    if (isOptional()) {
        auto defValIter = findValue(common::enumNullValueStr());
        assert(defValIter != m_values.cend());
        defValue = defValIter->first;
    }
    else {
        auto defValIter =
            std::min_element(
                m_values.begin(), m_values.end(),
                [](Values::const_reference v1, Values::const_reference v2) -> bool
                {
                    return std::abs(v1.first) < std::abs(v2.first);
                });
        assert(defValIter != m_values.end());
        defValue = defValIter->first;
    }

    auto writeRangesFunc =
        [&out, &ranges](unsigned ind)
            {
            for (auto& r : ranges) {
                out << ",\n" <<
                       output::indent(ind);
                if (r.first == r.second) {
                    out << "comms::option::ValidNumValue<" << common::num(r.first) << ">";
                }
                else {
                    out << "comms::option::ValidNumValueRange<" << common::num(r.first) << ", " << common::num(r.second) << ">";
                }
            }
        };

    auto writeDefaultValueFunc =
        [&out, defValue](unsigned ind)
        {
            if (defValue == 0) {
                return;
            }

            out << ",\n" <<
                   output::indent(ind) << "comms::option::DefaultNumValue<" << common::num(defValue) << ">";
        };

    if (asType) {
        out << output::indent(indent) << "struct " << name << " : public\n" <<
               output::indent(indent + 1) << "comms::field::EnumValue<\n" <<
               output::indent(indent + 2) << getFieldBaseString() << ",\n" <<
               output::indent(indent + 2) << enumName << ",\n" <<
               output::indent(indent + 2) << "TOpt...";
        writeDefaultValueFunc(indent + 2);
        writeRangesFunc(indent + 2);
        out << '\n' <<
               output::indent(indent + 1) << ">\n" <<
               output::indent(indent) << "{\n";
        common::writeDefaultSetVersionFunc(out, indent + 1);
        out << output::indent(indent) << "};\n\n";
        return;
    }

    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << "comms::field::EnumValue<\n" <<
           output::indent(indent + 2) << getFieldBaseString() << ",\n" <<
           output::indent(indent + 2) << enumName << ",\n" <<
           output::indent(indent + 2) << "TOpt...";
    writeDefaultValueFunc(indent + 2);
    if (!tooManyRanges) {
        writeRangesFunc(indent + 2);
    }
    out << '\n' <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    if (tooManyRanges) {
        out << output::indent(indent + 1) << "/// \\brief Custom implementation of validity check.\n" <<
               output::indent(indent + 1) << "bool valid() const\n" <<
               output::indent(indent + 1) << "{\n" <<
               output::indent(indent + 2) << common::fieldBaseDefStr() <<
               output::indent(indent + 2) << "static const " << enumName << " Values[] = {\n";
        std::intmax_t last = 0;
        bool firstValue = true;
        for (auto& v : m_values) {
            if ((!firstValue) && (v.first == last)) {
                continue;
            }

            firstValue = false;
            last = v.first;
            out << output::indent(indent + 3) << valuePrefix << v.second << ",\n";
        }
        out << output::indent(indent + 2) << "};\n\n" <<
               output::indent(indent + 2) << "if (!Base::valid()) {\n" <<
               output::indent(indent + 3) << "return false;\n" <<
               output::indent(indent + 2) << "}\n\n" <<
               output::indent(indent + 2) << "auto iter = std::lower_bound(std::begin(Values), std::end(Values), Base::value());\n" <<
               output::indent(indent + 2) << "return (iter != std::end(Values)) && (*iter == Base::value());\n" <<
               output::indent(indent + 1) << "}\n\n";
    }

    if (isOptional()) {
        common::writeEnumNullCheckUpdateFuncs(out, indent + 1);
    }

    out << '\n';
    common::writeDefaultSetVersionFunc(out, indent + 1);

    out << output::indent(indent) << "};\n\n";
}

void EnumType::writeList(
    std::ostream& out,
    unsigned indent,
    unsigned count,
    bool commsOptionalWrapped)
{
    writeHeader(out, indent, commsOptionalWrapped, true);
    common::writeExtraOptionsTemplParam(out, indent);
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto name = common::refName(getName(), suffix);
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << getFieldBaseString() << ",\n" <<
           output::indent(indent + 2) << common::refName(getName(), common::elementSuffixStr()) << "<>,\n" <<
           output::indent(indent + 2) << "TOpt...";
    if (count != 0U) {
        out << ",\n" <<
               output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << count << ">";
    }
    out << '\n' <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << output::indent(indent) << "};\n\n";
}

const std::string& EnumType::getUnderlyingType() const
{
    auto& encType = getEncodingType();
    if (encType.empty()) {
        log::error() << "Encoding type was NOT specified for enum \"" << getName() << "\"" << std::endl;
        return common::emptyString();
    }

    auto& types = getDb().getTypes();
    auto typeIter = types.find(encType);
    if (typeIter == types.end()) {
        if (encType == common::uint64Type()) {
            log::error() << "Support for uint64 as underlying enum type is currently not implemented." << std::endl;
            return common::emptyString();
        }

        return common::primitiveTypeToStdInt(encType);
    }

    assert(typeIter->second);
    if (typeIter->second->getKind() != Kind::Basic) {
        log::error() << "Only basic type can be used as encodingType for enum \"" << getName() << "\"" << std::endl;
        return common::emptyString();
    }

    auto& typePtr = typeIter->second;
    auto& basicType = static_cast<const BasicType&>(*typePtr);
    auto& primType = basicType.getPrimitiveType();
    if (primType.empty()) {
        log::error() << "Type \"" << encType << "\" used as encoding type for enum \"" << getName() <<
                     "\" doesn't specify primitiveType." << std::endl;
        return common::emptyString();
    }

    if (primType == common::uint64Type()) {
        log::error() << "Support for uint64 as underlying enum type is currently not implemented." << std::endl;
        return common::emptyString();
    }

    return common::primitiveTypeToStdInt(primType);
}

bool EnumType::readValues()
{
    static const std::string ElementStr("validValue");
    auto vals = xmlChildren(getNode(), ElementStr);
    if (vals.empty()) {
        log::error() << "No validValue has been specified for enum \"" << getName() << "\"" << std::endl;
        return false;
    }

    auto underlying = getUnderlyingType();
    bool isChar = (underlying == "char");
    std::set<std::string> processedNames;
    for (auto* v : vals) {
        auto vProps = xmlParseNodeProps(v, getDb().getDoc());
        auto& vName = prop::name(vProps);
        if (vName.empty()) {
            log::error() << "The enum \"" << getName() << "\" has validValue without name." << std::endl;
            return false;
        }

        auto nameIter = processedNames.find(vName);
        if (nameIter != processedNames.end()) {
            log::error() << "The enum \"" << getName() << "\" has at least two validValues with the same name (\"" << vName << "\")" << std::endl;
            return false;
        }

        auto text = xmlText(v);
        if (text.empty()) {
            log::error() << "The validValue \"" << vName << "\" of enum \"" << getName() << "\" doesn't specify the numeric value." << std::endl;
            return false;
        }

        if (!getDb().doesElementExist(prop::sinceVersion(vProps))) {
            continue;
        }

        std::intmax_t numVal = 0;
        do {
            if (isChar) {
                if (text.size() != 1U) {
                    log::error() << "Only single character char enums are supported in \"" << vName << "\" of enum \"" << getName() << "\" doesn't specify the numeric value." << std::endl;
                    return false;
                }

                numVal = static_cast<std::int8_t>(text[0]);
                break;
            }

            auto numValPair = stringToInt(text);
            if (!numValPair.second) {
                log::error() << "The validValue \"" << vName << "\" of enum \"" << getName() << "\" doesn't specify the numeric value." << std::endl;
                return false;
            }

            numVal = numValPair.first;
        } while (false);

        auto iter = m_values.find(numVal);
        if (iter != m_values.end()) {
            log::error() << "Failed to introduce value \"" << vName << "\" of enum \"" << getName() <<
                            "\" due to the numeric value being occupied by \"" << iter->second << "\"." << std::endl;
            return false;
        }

        m_values.insert(std::make_pair(numVal, vName));
        processedNames.insert(vName);

        auto& desc = prop::description(vProps);
        if (!desc.empty()) {
            m_desc.insert(std::make_pair(vName, desc));
        }
    }

    if (!isOptional()) {
        return !m_values.empty();
    }

    auto iter = processedNames.find(common::enumNullValueStr());
    if (iter != processedNames.end()) {
        log::error() << "Failed to introduce nullValue \"" << common::enumNullValueStr() <<
                     "\" due to the name being in use by the validValue." << std::endl;
        return false;
    }

    std::intmax_t nullVal = 0;
    do {
        auto& nullValueStr = getNullValue();
        assert(!nullValueStr.empty());

        if (isChar) {
            if (nullValueStr.size() != 1U) {
                log::error() << "Only single character char enums are supported nullValue of enum \"" << getName() << "\" doesn't specify the numeric value." << std::endl;
                return false;
            }

            nullVal = static_cast<std::int8_t>(nullValueStr[0]);
            break;
        }

        auto nullValPair = stringToInt(nullValueStr);
        if (!nullValPair.second) {
            log::error() << "Unknown nullValue format in enum \"" << getName() << "\"." << std::endl;
            return false;
        }

        nullVal = nullValPair.first;
    } while (false);

    m_values.insert(std::make_pair(nullVal, common::enumNullValueStr()));
    m_desc.insert(std::make_pair(common::enumNullValueStr(), "NULL value of optional field."));
    return !m_values.empty();
}

EnumType::RangeInfosList EnumType::getValidRanges() const
{
    RangeInfosList result;
    for (auto& v : m_values) {
        if (result.empty()) {
            result.push_back(std::make_pair(v.first, v.first));
            continue;
        }

        auto& last = result.back().second;
        if (v.first == (last + 1)) {
            last = v.first;
            continue;
        }

        result.push_back(std::make_pair(v.first, v.first));
    }
    return result;
}

EnumType::Values::const_iterator EnumType::findValue(const std::string& name) const
{
    for (auto iter = m_values.begin(); iter != m_values.end(); ++iter) {
        if (iter->second == name) {
            return iter;
        }
    }

    return m_values.end();
}

unsigned EnumType::getAdjustedLengthProp() const
{
    return 1U;
}

} // namespace sbe2comms
