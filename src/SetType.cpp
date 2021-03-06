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

#include "SetType.h"

#include <iostream>
#include <map>
#include <limits>
#include <cstdint>
#include <sstream>
#include <set>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"
#include "common.h"

namespace sbe2comms
{

namespace
{

std::uintmax_t getLenMask(unsigned len)
{
    assert(0U < len);
    auto lenMask = std::numeric_limits<std::uintmax_t>::max();
    if (len < sizeof(std::uintmax_t)) {
        lenMask = (1ULL << (len * std::numeric_limits<std::uint8_t>::digits)) - 1;
    }
    return lenMask;
}

} // namespace

SetType::Kind SetType::getKindImpl() const
{
    return Kind::Set;
}

bool SetType::parseImpl()
{
    if (!isRequired()) {
        log::error() << "The set \"" << getName() << "\" cannot be optional or constant." << std::endl;
        return false;
    }

    auto& encType = getEncodingType();
    if (encType.empty()) {
        log::error() << "Unknown encoding type for set \"" << getName() << "\"." << std::endl;
        return false;
    }

    if (getSerializationLength() == 0U) {
        log::error() << "Failed to identify length of the set \"" << getName() << "\", please check encoding type" << std::endl;
        return false;
    }

    if (!readChoices()) {
        return false;
    }

    if (getLengthProp() != 1U) {
        log::warning() << "Ignoring \"length\" property of \"" << getName() << "\" type to match sbe-tool." << std::endl;
    }

    addExtraInclude("\"comms/field/BitmaskValue.h\"");
    return true;
}

bool SetType::writeImpl(std::ostream& out, unsigned indent, bool commsOptionalWrapped)
{
    assert(0U < getSerializationLengthImpl());

    auto count = getAdjustedLengthProp();
    if (count != 1U) {
        writeSingle(out, indent, commsOptionalWrapped, true);
    }

    if (count == 1U) {
        writeSingle(out, indent, commsOptionalWrapped);
        return true;
    }

    writeList(out, indent, commsOptionalWrapped, count);

    return true;
}

std::size_t SetType::getSerializationLengthImpl() const
{
    auto& encType = getEncodingType();
    assert(!encType.empty());

    auto& types = getDb().getTypes();
    auto iter = types.find(encType);
    if (iter == types.end()) {
        auto len = primitiveLength(encType);
        if (len == 0) {
            log::error() << "Unknown encoding type \"" << encType << "\" for set \"" <<
                            getName() << "\"" << std::endl;
        }

        return len;
    }

    assert(iter->second);
    auto k = iter->second->getKind();
    if (k != Kind::Basic) {
        log::error() << "Only basic type can be used as encodingType for set \"" <<
                        getName() << "\"" << std::endl;
        return 0U;
    }

    return iter->second->getSerializationLength();
}

bool SetType::hasFixedLengthImpl() const
{
    return getAdjustedLengthProp() != 0U;
}

bool SetType::writePluginPropertiesImpl(
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
    for (auto& b : m_bits) {
        out << '\n' <<
               output::indent(indent + 2) << ".add(" << b.first << ", \"" << b.second << "\")";
    }
    out << ";\n\n";

    writeSerialisedHiddenCheck(out, indent, props);

    if (scope.empty() && (!commsOptionalWrapped)) {
        out << output::indent(indent) << "return " << props << ".asMap();\n";
    }

    return true;
}

void SetType::writeSingle(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped,
    bool isElement)
{
    if (isElement) {
        writeElementHeader(out, indent);
    }
    else {
        writeHeader(out, indent, commsOptionalWrapped, true);
    }
    common::writeExtraOptionsTemplParam(out, indent);

    auto& suffix = getNameSuffix(commsOptionalWrapped, isElement);
    auto name = common::refName(getName(), suffix);
    auto len = getSerializationLengthImpl();
    auto reservedMask = calcReservedMask(len);

    auto writeClassDefFunc =
        [this, &out, len, reservedMask](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::BitmaskValue<\n" <<
                   output::indent(ind + 1) << common::fieldBaseFullScope(getDb().getProtocolNamespace()) << ",\n" <<
                   output::indent(ind + 1) << "TOpt...,\n" <<
                   output::indent(ind + 1) << "comms::option::FixedLength<" << len << ">";

         if (reservedMask != 0U) {
             out << ",\n" <<
                    output::indent(ind + 1) << "comms::option::BitmaskReservedBits<0x" << std::hex << reservedMask << std::dec << ">";
         }

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

    auto maskTmp = getLenMask(len) & (~reservedMask);
    bool isSeq = (((maskTmp + 1) & (maskTmp)) == 0);
    if (isSeq) {
        writeSeq(out, indent + 1);
    }
    else {
        writeNonSeq(out, indent + 1);
    }

    out << '\n';
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << "};\n\n";
}

void SetType::writeList(
    std::ostream& out,
    unsigned indent,
    bool commsOptionalWrapped,
    unsigned count)
{
    writeHeader(out, indent, commsOptionalWrapped, true);
    common::writeExtraOptionsTemplParam(out, indent);
    auto& suffix = getNameSuffix(commsOptionalWrapped, false);
    auto writeClassDefFunc =
        [this, &out, count](unsigned ind)
        {
            out << output::indent(ind) << "comms::field::ArrayList<\n" <<
                   output::indent(ind + 1) << common::fieldBaseFullScope(getDb().getProtocolNamespace()) << ",\n" <<
                   output::indent(ind + 1) << getName() << common::elementSuffixStr() << "<>,\n" <<
                   output::indent(ind + 1) << "TOpt...";
            if (count != 0U) {
                out << ",\n" <<
                       output::indent(ind + 1) << "comms::option::SequenceFixedSize<" << count << ">";
            }
            out << '\n' <<
                   output::indent(ind) << ">";

        };

    out << output::indent(indent) << "class " << common::refName(getName(), suffix) << " : public\n";
    writeClassDefFunc(indent + 1);
    out << '\n' <<
           output::indent(indent) << "{\n" <<
           output::indent(indent + 1) << "using Base=\n";
    writeClassDefFunc(indent + 2);
    out << ";\n\n" <<
           output::indent(indent) << "public:\n";
    common::writeDefaultSetVersionFunc(out, indent + 1);
    out << "};\n\n";
}

bool SetType::readChoices()
{
    static const std::string ElementStr("choice");
    auto choices = xmlChildren(getNode(), ElementStr);
    if (choices.empty()) {
        log::error() << "No choice has been specified for set \"" << getName() << "\"" << std::endl;
        return false;
    }

    std::size_t bitsCount = getSerializationLength() * std::numeric_limits<std::uint8_t>::digits;

    static const std::size_t MaxBitsLimit = 64U;
    if (MaxBitsLimit < bitsCount) {
        log::error() << "Maximum allowed amount of bits for \"" << getName() << "\" set is 64." << std::endl;
        return false;
    }

    assert(0U < bitsCount);
    std::set<std::string> processedNames;
    for (auto* c : choices) {
        auto choiceProps = xmlParseNodeProps(c, getDb().getDoc());
        auto& choiceName = prop::name(choiceProps);
        if (choiceName.empty()) {
            log::error() << "The set \"" << getName() << "\" has choice without name." << std::endl;
            return false;
        }

        auto nameIter = processedNames.find(choiceName);
        if (nameIter != processedNames.end()) {
            log::error() << "The set \"" << getName() << "\" has at least two choices with the same name (\"" << choiceName << "\")" << std::endl;
            return false;
        }

        auto text = xmlText(c);
        if (text.empty()) {
            log::error() << "The choice \"" << choiceName << "\" of set \"" << getName() << "\" doesn't specify the bit number." << std::endl;
            return false;
        }

        auto bitIdx = stringToInt(text);
        if (!bitIdx.second) {
            log::error() << "The choice \"" << choiceName << "\" of set \"" << getName() << "\" doesn't specify the numeric bit number." << std::endl;
            return false;
        }

        if (bitIdx.first < 0) {
            log::error() << "The choice \"" << choiceName << "\" of set \"" << getName() << "\" specifies negative bit number." << std::endl;
            return false;
        }

        auto castedBitIdx = static_cast<unsigned>(bitIdx.first);
        if (bitsCount <= castedBitIdx) {
            log::error() << "The choice \"" << choiceName << "\" of set\"" << getName() << "\" specifies bit number, which is too high." << std::endl;
            return false;
        }

        auto bitsIter = m_bits.find(castedBitIdx);
        if (bitsIter != m_bits.end()) {
            log::error() << "The set \"" << getName() << "\" has at least two choices with the same bit index." << std::endl;
            return false;
        }

        if (getDb().doesElementExist(prop::sinceVersion(choiceProps))) {
            m_bits.insert(std::make_pair(castedBitIdx, choiceName));
            processedNames.insert(choiceName);
        }
    }

    return true;
}

void SetType::writeSeq(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "/// \\brief Provide names and convenience access functions for internal bits.\n" <<
           output::indent(indent) << "/// \\details See definition of \\b COMMS_BITMASK_BITS_SEQ macro\n" <<
           output::indent(indent) << "///     related to \\b comms::field::BitmaskValue class from COMMS library\n" <<
           output::indent(indent) << "///     for details.\n" <<
           output::indent(indent) << "COMMS_BITMASK_BITS_SEQ(\n";
    bool first = true;
    for (auto& b : m_bits) {
        if (!first) {
            out << ",\n";
        }
        else {
            first = false;
        }

        out << output::indent(indent + 1) << b.second;
    }
    out << "\n" << output::indent(indent) << ");\n";
}

void SetType::writeNonSeq(std::ostream& out, unsigned indent)
{
    out << output::indent(indent) << "/// \\brief Provide names for internal bits.\n" <<
           output::indent(indent) << "/// \\details See definition of \\b COMMS_BITMASK_BITS macro\n" <<
           output::indent(indent) << "///     related to \\b comms::field::BitmaskValue class from COMMS library\n" <<
           output::indent(indent) << "///     for details.\n" <<
           output::indent(indent) << "COMMS_BITMASK_BITS(\n";
    bool first = true;
    for (auto& b : m_bits) {
        if (!first) {
            out << ",\n";
        }
        else {
            first = false;
        }

        out << output::indent(indent + 1) << b.second << "=" << b.first;
    }
    out << "\n" << output::indent(indent) << ");\n\n";

    out << output::indent(indent) << "/// \\brief Provide convenience access functions for internal bits.\n" <<
           output::indent(indent) << "/// \\details See definition of \\b COMMS_BITMASK_BITS_ACCESS macro\n" <<
           output::indent(indent) << "///     related to \\b comms::field::BitmaskValue class from COMMS library\n" <<
           output::indent(indent) << "///     for details.\n" <<
           output::indent(indent) << "COMMS_BITMASK_BITS_ACCESS(\n";
    first = true;
    for (auto& b : m_bits) {
        if (!first) {
            out << ",\n";
        }
        else {
            first = false;
        }

        out << output::indent(indent + 1) << b.second;
    }
    out << "\n" << output::indent(indent) << ");\n";
}

std::uintmax_t SetType::calcReservedMask(unsigned len)
{
    auto mask = std::numeric_limits<std::uintmax_t>::max() & getLenMask(len);

    for (auto& b : m_bits) {
        mask &= ~(1ULL << b.first);
    }
    return mask;
}

unsigned SetType::getAdjustedLengthProp() const
{
    return 1U;
}


} // namespace sbe2comms
