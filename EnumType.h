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
#include <cstdint>

#include "Type.h"

namespace sbe2comms
{

class EnumType : public Type
{
    using Base = Type;
public:
    explicit EnumType(DB& db, xmlNodePtr node) : Base(db, node) {}

protected:
    virtual Kind kindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent) override;
    virtual std::size_t getSerializationLengthImpl() const override;
    virtual bool hasListOrStringImpl() const override;

private:
    using Values = std::multimap<std::intmax_t, std::string>;
    using Descriptions = std::map<std::string, std::string>;
    using RangeInfo = std::pair<std::intmax_t, std::intmax_t>;
    using RangeInfosList = std::list<RangeInfo>;

    void writeSingle(std::ostream& out, unsigned indent, bool isElement = false);
    void writeList(std::ostream& out, unsigned indent, unsigned count);
    const std::string& getUnderlyingType() const;
    bool readValues();
    RangeInfosList getValidRanges() const;

    Values m_values;
    Descriptions m_desc;
};

} // namespace sbe2comms
