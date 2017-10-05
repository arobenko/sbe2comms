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

#include "get.h"
#include "output.h"
#include "log.h"
#include "BasicType.h"

namespace sbe2comms
{

namespace
{

const std::string ElementSuffix("Element");
const std::string NullValueName("NullValue");
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

EnumType::Kind EnumType::kindImpl() const
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

    auto ranges = getValidRanges();
    if (MaxRangesCount < ranges.size()) {
        addExtraInclude("<algorithm>");
    }

    return true;
}

bool EnumType::writeImpl(std::ostream& out, unsigned indent)
{
    auto count = getLengthProp();
    if (count != 1U) {
        writeSingle(out, indent, true);
    }

    if (count == 1U) {
        writeSingle(out, indent);
        return true;
    }

    writeList(out, indent, count);

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
    auto k = iter->second->kind();
    if (k != Kind::Basic) {
        log::error() << "Only basic type can be used as encodingType for enum \"" << getName() << "\"" << std::endl;
        return 0U;
    }

    return iter->second->getSerializationLength();
}

bool EnumType::hasListOrStringImpl() const
{
    return getLengthProp() != 1U;
}

void EnumType::writeSingle(std::ostream& out, unsigned indent, bool isElement)
{
    auto& underlying = getUnderlyingType();
    assert(!underlying.empty());

    auto enumName = getName() + "Val";
    out << output::indent(indent) << "/// \\brief Enumeration for \"" << getName() << "\" field.\n" <<
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

    auto name = getName();
    if (isElement) {
        writeBriefElement(out, indent);
        name += ElementSuffix;
    }
    else {
        writeBrief(out, indent, true);
    }
    writeOptions(out, indent);

    auto ranges = getValidRanges();
    bool tooManyRanges = MaxRangesCount < ranges.size();
    bool asType = (!isOptional()) && (!tooManyRanges);

    auto writeRangesFunc =
        [&out, &ranges](unsigned ind)
            {
            for (auto& r : ranges) {
                out << output::indent(ind);
                if (r.first == r.second) {
                    out << "comms::option::ValidNumValue<" << toString(r.first) << ">,\n";
                }
                else {
                    out << "comms::option::ValidNumValueRange<" << toString(r.first) << ", " << toString(r.second) << ">,\n";
                }
            }
        };

    if (asType) {
        out << output::indent(indent) << "using " << name << " = \n" <<
               output::indent(indent + 1) << "comms::field::EnumValue<\n" <<
               output::indent(indent + 2) << "FieldBase,\n" <<
               output::indent(indent + 2) << enumName << ",\n";
        writeRangesFunc(indent + 2);
        out << output::indent(indent + 2) << "TOpt...\n" <<
               output::indent(indent + 1) << ">;\n\n";
        return;
    }

    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << "comms::field::EnumValue<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << enumName << ",\n";
    if (!tooManyRanges) {
        writeRangesFunc(indent + 2);
    }
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";
    if (tooManyRanges) {
        out << output::indent(indent + 1) << "/// \\brief Custom implementation of validity check.\n" <<
               output::indent(indent + 1) << "bool valid() const\n" <<
               output::indent(indent + 1) << "{\n" <<
               output::indent(indent + 2) << "using Base = typename std::decay<decltype(toFieldBase(*this))>::type;\n" <<
               output::indent(indent + 2) << "static const " << enumName << " Values[] = {\n";
        std::intmax_t last = 0;
        bool firstValue = true;
        for (auto& v : m_values) {
            if ((!firstValue) && (v.first == last)) {
                continue;
            }

            firstValue = false;
            last = v.first;
            out << output::indent(indent + 3) << enumName << "::" << v.second << ",\n";
        }
        out << output::indent(indent + 2) << "};\n\n" <<
               output::indent(indent + 2) << "auto iter = std::lower_bound(std::begin(Values), std::end(Values), Base::value());\n" <<
               output::indent(indent + 2) << "return (iter != std::end(Values)) && (*iter == Base::value());\n" <<
               output::indent(indent + 1) << "}\n\n";
    }

    if (isOptional()) {
        out << output::indent(indent + 1) << "/// \\brief Check the value is equivalent to \\b nullValue.\n" <<
               output::indent(indent + 1) << "bool isNull() const\n" <<
               output::indent(indent + 1) << "{\n";
        writeBaseDef(out, indent + 2);
        out << output::indent(indent + 2) << "return Base::value() == Base::ValueType::" << NullValueName << ";\n" <<
               output::indent(indent + 1) << "}\n\n";
    }

    out << output::indent(indent) << "};\n\n";
}

void EnumType::writeList(std::ostream& out, unsigned indent, unsigned count)
{
    writeBrief(out, indent, true);
    writeOptions(out, indent);

    out << output::indent(indent) << "using " << getName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << getName() << ElementSuffix << "<>,\n";
    if (count != 0U) {
        out << output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << count << ">,\n";
    }
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">;\n\n";
}

const std::string& EnumType::getUnderlyingType() const
{
    auto& encType = getEncodingType();
    if (encType.empty()) {
        log::error() << "Encoding type was NOT specified for enum \"" << getName() << "\"" << std::endl;
        return get::emptyString();
    }

    auto& types = getDb().getTypes();
    auto typeIter = types.find(encType);
    if (typeIter == types.end()) {
        return primitiveTypeToStdInt(encType);
    }

    assert(typeIter->second);
    if (typeIter->second->kind() != Kind::Basic) {
        log::error() << "Only basic type can be used as encodingType for enum \"" << getName() << "\"" << std::endl;
        return get::emptyString();
    }

    auto& typePtr = typeIter->second;
    auto& basicType = static_cast<const BasicType&>(*typePtr);
    auto& primType = basicType.getPrimitiveType();
    if (primType.empty()) {
        log::error() << "Type \"" << encType << "\" used as encoding type for enum \"" << getName() <<
                     "\" doesn't specify primitiveType." << std::endl;
        return get::emptyString();
    }

    return primitiveTypeToStdInt(primType);
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

        if (getDb().doesElementExist(prop::sinceVersion(vProps), prop::deprecated(vProps))) {
            m_values.insert(std::make_pair(numVal, vName));
            processedNames.insert(vName);

            auto& desc = prop::description(vProps);
            if (!desc.empty()) {
                m_desc.insert(std::make_pair(vName, desc));
            }
        }
    }

    if (!isOptional()) {
        return !m_values.empty();
    }

    auto iter = processedNames.find(NullValueName);
    if (iter != processedNames.end()) {
        log::error() << "Failed to introduce nullValue \"" << NullValueName <<
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

    m_values.insert(std::make_pair(nullVal, NullValueName));
    m_desc.insert(std::make_pair(NullValueName, "NULL value of optional field."));
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

} // namespace sbe2comms
