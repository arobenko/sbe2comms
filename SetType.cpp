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

SetType::Kind SetType::kindImpl() const
{
    return Kind::Set;
}

bool SetType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    auto len = length(db);

    auto& p = props(db);
    if (len == 0U) {
        std::cerr << "ERROR: Failed to determine length for set \"" << prop::name(p) << "\"" << std::endl;
        return false;
    }

    if (!readChoices(db)) {
        return false;
    }

    auto reservedMask = calcReservedMask(len);
    writeBrief(out, db, indent);
    out << output::indent(indent) << "struct " << prop::name(p) << " : public\n" <<
           output::indent(indent + 1) << "comms::field::BitmaskValue<\n" <<
           output::indent(indent + 2) << "FieldBase,\n" <<
           output::indent(indent + 2) << "comms::option::FixedLength<" << len << ">";
    if (reservedMask != 0U) {
        std::stringstream stream;
        stream << std::hex << "0x" << reservedMask;
        out << ",\n" <<
               output::indent(indent + 2) << "comms::option::BitmaskReservedBits<" << stream.str() << ">";
    }
    out << "\n" <<
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
    return true;
}

std::size_t SetType::lengthImpl(DB& db)
{
    auto& p = props(db);
    auto& encType = prop::encodingType(p);
    if (encType.empty()) {
        std::cerr << "ERROR: Encoding type was NOT specified for set \"" << prop::name(p) << "\"" << std::endl;
        return 0U;
    }

    auto iter = db.m_types.find(encType);
    if (iter == db.m_types.end()) {
        auto len = primitiveLength(encType);
        if (len == 0) {
            std::cerr << "ERROR: Unknown encoding type \"" << encType << "\" for set \"" << prop::name(p) << "\"" << std::endl;
        }

        return len;
    }

    assert(iter->second);
    auto k = iter->second->kind();
    if (k != Kind::Basic) {
        std::cerr << "ERROR: Only basic type can be used as encodingType for set \"" << prop::name(p) << "\"" << std::endl;
        return 0U;
    }

    return iter->second->length(db);
}

bool SetType::readChoices(DB& db)
{
    static const std::string ElementStr("choice");
    auto choices = xmlChildren(getNode(), ElementStr);
    auto& p = props(db);
    if (choices.empty()) {
        std::cerr << "ERROR: No choice has been specified for set \"" << prop::name(p) << "\"" << std::endl;
        return false;
    }

    std::size_t bitsCount = length(db) * std::numeric_limits<std::uint8_t>::digits;
    assert(0U < bitsCount);
    std::set<std::string> processedNames;
    for (auto* c : choices) {
        auto choiceProps = xmlParseNodeProps(c, db.m_doc.get());
        auto& choiceName = prop::name(choiceProps);
        if (choiceName.empty()) {
            std::cerr << "ERROR: The set \"" << prop::name(p) << "\" has choice without name." << std::endl;
            return false;
        }

        auto nameIter = processedNames.find(choiceName);
        if (nameIter != processedNames.end()) {
            std::cerr << "ERROR: The set \"" << prop::name(p) << "\" has at least two choices with the same name (\"" << choiceName << "\")" << std::endl;
            return false;
        }

        auto text = xmlText(c);
        if (text.empty()) {
            std::cerr << "ERROR: The choice \"" << choiceName << "\" of set \"" << prop::name(p) << "\" doesn't specify the bit number." << std::endl;
            return false;
        }

        auto bitIdx = stringToInt(text);
        if (!bitIdx.second) {
            std::cerr << "ERROR: The choice \"" << choiceName << "\" of set \"" << prop::name(p) << "\" doesn't specify the numeric bit number." << std::endl;
            return false;
        }

        if (bitIdx.first < 0) {
            std::cerr << "ERROR: The choice \"" << choiceName << "\" of set \"" << prop::name(p) << "\" specifies negative bit number." << std::endl;
            return false;
        }

        auto castedBitIdx = static_cast<unsigned>(bitIdx.first);
        if (bitsCount <= castedBitIdx) {
            std::cerr << "ERROR: The choice \"" << choiceName << "\" of set\"" << prop::name(p) << "\" specifies bit number, which is too high." << std::endl;
            return false;
        }

        auto bitsIter = m_bits.find(castedBitIdx);
        if (bitsIter != m_bits.end()) {
            std::cerr << "ERROR: The set \"" << prop::name(p) << "\" has at least two choices with the same bit index." << std::endl;
            return false;
        }

        m_bits.insert(std::make_pair(castedBitIdx, choiceName));
        processedNames.insert(choiceName);
    }
    return true;
}

std::uintmax_t SetType::calcReservedMask(unsigned len)
{
    auto mask = std::numeric_limits<std::uintmax_t>::max() & getLenMask(len);

    for (auto& b : m_bits) {
        mask &= ~(1ULL << b.first);
    }
    return mask;
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


} // namespace sbe2comms
