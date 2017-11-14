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

    return true;
}

bool SetType::writeImpl(std::ostream& out, unsigned indent)
{
    auto serLen = getSerializationLengthImpl();
    assert(0U < serLen);

    auto count = getAdjustedLengthProp();
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

void SetType::writeSingle(std::ostream& out, unsigned indent, bool isElement)
{
    auto name = getName();
    if (isElement) {
        writeElementHeader(out, indent);
        name += common::elementSuffixStr();
    }
    else {
        writeHeader(out, indent, true);
    }
    common::writeExtraOptionsTemplParam(out, indent);

    auto len = getSerializationLengthImpl();
    out << output::indent(indent) << "struct " << name << " : public\n" <<
           output::indent(indent + 1) << "comms::field::BitmaskValue<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << "TOpt...,\n" <<
           output::indent(indent + 2) << "comms::option::FixedLength<" << len << ">";

    auto reservedMask = calcReservedMask(len);
    if (reservedMask != 0U) {
        std::stringstream stream;
        stream << std::hex << "0x" << reservedMask;
        out << ",\n" <<
               output::indent(indent + 2) << "comms::option::BitmaskReservedBits<" << stream.str() << ">";
    }

    out << '\n' <<
           output::indent(indent + 1) << ">\n" <<
           output::indent(indent) << "{\n";

    auto maskTmp = getLenMask(len) & (~reservedMask);
    bool isSeq = (((maskTmp + 1) & (maskTmp)) == 0);
    if (isSeq) {
        writeSeq(out, indent + 1);
    }
    else {
        writeNonSeq(out, indent + 1);
    }
    out << "};\n\n";
}

void SetType::writeList(std::ostream& out, unsigned indent, unsigned count)
{
    writeHeader(out, indent, true);
    common::writeExtraOptionsTemplParam(out, indent);

    out << output::indent(indent) << "using " << getReferenceName() << " = \n" <<
           output::indent(indent + 1) << "comms::field::ArrayList<\n" <<
           output::indent(indent + 2) << common::fieldBaseStr() << ",\n" <<
           output::indent(indent + 2) << getName() << common::elementSuffixStr() << "<>,\n" <<
           output::indent(indent + 2) << "TOpt...";
    if (count != 0U) {
        out << ",\n" <<
               output::indent(indent + 2) << "comms::option::SequenceFixedSize<" << count << ">";
    }
    out << '\n' <<
           output::indent(indent + 1) << ">;\n\n";
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