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

namespace sbe2comms
{

EnumType::Kind EnumType::kindImpl() const
{
    return Kind::Enum;
}

bool EnumType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    auto& p = props(db);
    if (prop::isConstant(p)) {
        std::cerr << "ERROR: Enum type \"" << prop::name(p) << "\" cannot be constant." << std::endl;
        return false;
    }

    auto& underlying = getUnderlyingType(db);
    if (underlying.empty()) {
        return false;
    }

    if (!readValues(db)) {
        return false;
    }

    auto enumName = prop::name(p) + "Val";
    out << output::indent(indent) << "/// \\brief Enumeration for \"" << prop::name(p) << "\" field.\n" <<
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

    static const std::size_t MaxRangesCount = 10;
    auto ranges = getValidRanges();
    bool tooManyRanges = MaxRangesCount < ranges.size();
    writeBrief(out, db, indent, true);
    out << output::indent(indent) << "template <typename... TOpt>\n";
    if (tooManyRanges) {
        out << output::indent(indent) << "struct " << prop::name(p) << " : public\n" <<
               output::indent(indent + 1) << "comms::field::EnumValue<\n" <<
               output::indent(indent + 2) << "FieldBase,\n" <<
               output::indent(indent + 2) << enumName << ",\n" <<
               output::indent(indent + 2) << "TOpt...\n" <<
               output::indent(indent + 1) << ">\n" <<
               output::indent(indent) << "{\n" <<
               output::indent(indent + 1) << "/// \\brief Custom implementation of validity check.\n" <<
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
               output::indent(indent + 1) << "}\n" <<
               output::indent(indent) << "};\n\n";
        return true;
    }

    out << output::indent(indent) << "using " << prop::name(p) << " = \n" <<
           output::indent(indent + 1) << "comms::field::EnumValue<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << enumName << ",\n";
    for (auto& r : ranges) {
        out << output::indent(indent + 2);
        if (r.first == r.second) {
            out << "comms::option::ValidNumValue<" << toString(r.first) << ">,\n";
        }
        else {
            out << "comms::option::ValidNumValueRange<" << toString(r.first) << ", " << toString(r.second) << ">,\n";
        }
    }
    out << output::indent(indent + 2) << "TOpt...\n" <<
           output::indent(indent + 1) << ">;\n\n";
    return true;
}

std::size_t EnumType::lengthImpl(DB& db)
{
    auto& p = props(db);
    auto& encType = prop::encodingType(p);
    if (encType.empty()) {
        std::cerr << "ERROR: Encoding type was NOT specified for enum \"" << prop::name(p) << "\"" << std::endl;
        return 0U;
    }


    auto iter = db.m_types.find(encType);
    if (iter == db.m_types.end()) {
        auto len = primitiveLength(encType);
        if (len == 0) {
            std::cerr << "ERROR: Unknown encoding type \"" << encType << "\" for enum \"" << prop::name(p) << "\"" << std::endl;
        }

        return len;
    }

    assert(iter->second);
    auto k = iter->second->kind();
    if (k != Kind::Basic) {
        std::cerr << "ERROR: Only basic type can be used as encodingType for enum \"" << prop::name(p) << "\"" << std::endl;
        return 0U;
    }

    return iter->second->length(db);
}

const std::string& EnumType::getUnderlyingType(DB& db)
{
    auto& p = props(db);
    auto& encType = prop::encodingType(p);
    if (encType.empty()) {
        std::cerr << "ERROR: Encoding type was NOT specified for enum \"" << prop::name(p) << "\"" << std::endl;
        return get::emptyString();
    }

    auto typeIter = db.m_types.find(encType);
    if (typeIter == db.m_types.end()) {
        return primitiveTypeToStdInt(encType);
    }

    assert(typeIter->second);
    if (typeIter->second->kind() != Kind::Basic) {
        std::cerr << "ERROR: Only basic type can be used as encodingType for enum \"" << prop::name(p) << "\"" << std::endl;
        return get::emptyString();
    }

    auto typeProps = typeIter->second->props(db);
    auto& primType = prop::primitiveType(typeProps);
    if (primType.empty()) {
        std::cerr << "ERROR: Type \"" << encType << "\" used as encoding type for enum \"" << prop::name(p) <<
                     "\" doesn't specify primitiveType." << std::endl;
        return get::emptyString();
    }

    return primitiveTypeToStdInt(primType);
}

bool EnumType::readValues(DB& db)
{
    static const std::string ElementStr("validValue");
    auto vals = xmlChildren(getNode(), ElementStr);
    auto& p = props(db);
    if (vals.empty()) {
        std::cerr << "ERROR: No validValue has been specified for enum \"" << prop::name(p) << "\"" << std::endl;
        return false;
    }

    auto underlying = getUnderlyingType(db);
    bool isChar = (underlying == "char");
    std::set<std::string> processedNames;
    for (auto* v : vals) {
        auto vProps = xmlParseNodeProps(v, db.m_doc.get());
        auto& vName = prop::name(vProps);
        if (vName.empty()) {
            std::cerr << "ERROR: The enum \"" << prop::name(p) << "\" has validValue without name." << std::endl;
            return false;
        }

        auto nameIter = processedNames.find(vName);
        if (nameIter != processedNames.end()) {
            std::cerr << "ERROR: The enum \"" << prop::name(p) << "\" has at least two validValues with the same name (\"" << vName << "\")" << std::endl;
            return false;
        }

        auto text = xmlText(v);
        if (text.empty()) {
            std::cerr << "ERROR: The validValue \"" << vName << "\" of enum \"" << prop::name(p) << "\" doesn't specify the numeric value." << std::endl;
            return false;
        }

        std::intmax_t numVal = 0;
        do {
            if (isChar) {
                if (text.size() != 1U) {
                    std::cerr << "ERROR: Only single character char enums are supported in \"" << vName << "\" of enum \"" << prop::name(p) << "\" doesn't specify the numeric value." << std::endl;
                    return false;
                }

                numVal = static_cast<std::int8_t>(text[0]);
                break;
            }

            auto numValPair = stringToInt(text);
            if (!numValPair.second) {
                std::cerr << "ERROR: The validValue \"" << vName << "\" of enum \"" << prop::name(p) << "\" doesn't specify the numeric value." << std::endl;
                return false;
            }

            numVal = numValPair.first;
        } while (false);

        m_values.insert(std::make_pair(numVal, vName));
        processedNames.insert(vName);

        auto& desc = prop::description(p);
        if (!desc.empty()) {
            m_desc.insert(std::make_pair(vName, desc));
        }
    }

    if (!prop::isOptional(p)) {
        return true;
    }

    static const std::string NullValueName("NullValue");
    auto iter = processedNames.find(NullValueName);
    if (iter != processedNames.end()) {
        std::cerr << "ERROR: Failed to introduce nullValue \"" << NullValueName <<
                     "\" due to the name being in use by the validValue." << std::endl;
        return false;
    }

    auto& nullValueStr = prop::nullValue(p);
    if (nullValueStr.empty()) {
        m_values.insert(std::make_pair(builtInIntNullValue(underlying), NullValueName));
        return true;
    }

    auto nullVal = stringToInt(nullValueStr);
    if (!nullVal.second) {
        std::cerr << "ERROR: Unknown nullValue format in enum \"" << prop::name(p) << "\"." << std::endl;
        return false;
    }

    m_values.insert(std::make_pair(nullVal.first, NullValueName));
    m_desc.insert(std::make_pair(NullValueName, "NULL value of optional field."));
    return true;
}

EnumType::RangeInfosList EnumType::getValidRanges()
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

} // namespace sbe2comms
