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

#pragma once

#include <map>

#include "Type.h"

namespace sbe2comms
{

class SetType : public Type
{
    using Base = Type;
public:
    explicit SetType(DB& db, xmlNodePtr node) : Base(db, node) {}

protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent, const std::string& suffix) override;
    virtual std::size_t getSerializationLengthImpl() const override;
    virtual bool hasFixedLengthImpl() const override;
    virtual bool writePluginPropertiesImpl(
        std::ostream& out,
        unsigned indent,
        const std::string& scope) override;

private:
    using BitsMap = std::map<unsigned, std::string>;

    void writeSingle(
        std::ostream& out,
        unsigned indent,
        const std::string& suffix,
        bool isElement = false);
    void writeList(
        std::ostream& out,
        unsigned indent,
        const std::string& suffix,
        unsigned count);
    bool readChoices();
    void writeSeq(std::ostream& out, unsigned indent);
    void writeNonSeq(std::ostream& out, unsigned indent);
    std::uintmax_t calcReservedMask(unsigned len);
    unsigned getAdjustedLengthProp() const;

    BitsMap m_bits;
};

} // namespace sbe2comms
