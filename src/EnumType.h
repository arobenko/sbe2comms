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

    bool hasValue(const std::string& name) const;
    std::intmax_t getNumericValue(const std::string& name) const;
    std::intmax_t getDefultNullValue() const;
    void setMessageId()
    {
        m_msgId = true;
    }

protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent) override;
    virtual std::size_t getSerializationLengthImpl() const override;
    virtual bool hasFixedLengthImpl() const override;
    virtual bool canBeExtendedAsOptionalImpl() const override;

private:
    using Values = std::map<std::intmax_t, std::string>;
    using Descriptions = std::map<std::string, std::string>;
    using RangeInfo = std::pair<std::intmax_t, std::intmax_t>;
    using RangeInfosList = std::list<RangeInfo>;

    void writeEnumVal(std::ostream& out, unsigned indent);
    void writeSingle(std::ostream& out, unsigned indent, bool isElement = false);
    void writeList(std::ostream& out, unsigned indent, unsigned count);
    const std::string& getUnderlyingType() const;
    bool readValues();
    RangeInfosList getValidRanges() const;
    Values::const_iterator findValue(const std::string& name) const;
    unsigned getAdjustedLengthProp() const;

    Values m_values;
    Descriptions m_desc;
    bool m_msgId = false;
};

inline
EnumType& asEnumType(Type& type)
{
    return static_cast<EnumType&>(type);
}

inline
const EnumType* asEnumType(const Type* type)
{
    return static_cast<const EnumType*>(type);
}


} // namespace sbe2comms
