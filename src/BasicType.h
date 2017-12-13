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

#include "Type.h"

namespace sbe2comms
{

class BasicType : public Type
{
    using Base = Type;
public:
    explicit BasicType(DB& db, xmlNodePtr node) : Base(db, node) {}

    const std::string& getPrimitiveType() const;
    std::intmax_t getDefultIntNullValue() const;
    bool isIntType() const;
    bool isFpType() const;

protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent, bool commsOptionalWrapped) override;
    virtual std::size_t getSerializationLengthImpl() const override;
    virtual bool hasFixedLengthImpl() const override;
    virtual bool canBeExtendedAsOptionalImpl() const override;
    virtual bool writePluginPropertiesImpl(
        std::ostream& out,
        unsigned indent,
        const std::string& scope) override;

private:
    bool writeSimpleType(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped,
        bool isElement = false);

    bool writeSimpleBigUnsignedInt(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped,
        bool isElement,
        std::uintmax_t minVal,
        std::uintmax_t maxVal);

    bool writeSimpleInt(
        std::ostream& out,
        unsigned indent,
        const std::string& intType,
        bool commsOptionalWrapped,
        bool isElement);

    bool writeSimpleFloat(
        std::ostream& out,
        unsigned indent,
        const std::string& fpType,
        bool commsOptionalWrapped,
        bool isElement);

    bool writeVarLength(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped);

    bool writeVarLengthString(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped);

    bool writeVarLengthArray(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped);

    bool writeVarLengthRawDataArray(
        std::ostream& out,
        unsigned indent,
        const std::string& primType,
        bool commsOptionalWrapped);

    bool writeFixedLength(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped);

    bool writeFixedLengthString(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped);

    bool writeFixedLengthArray(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped);

    bool writeFixedLengthRawDataArray(
        std::ostream& out,
        unsigned indent,
        const std::string& primType,
        bool commsOptionalWrapped);

    bool isString() const;

    bool isConstString() const;

    bool isRawData() const;

    bool isRawData(const std::string& primType) const;

    void writeStringValidFunc(std::ostream& out, unsigned indent);

    bool hasDefaultValueInExtraOptions() const;

    bool writePluginPropertiesSimple(
        std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool isElement,
        int index = -1);

    bool writePluginPropertiesList(
        std::ostream& out,
        unsigned indent,
        const std::string& scope);
};

inline
const BasicType* asBasicType(const Type* type)
{
    return static_cast<const BasicType*>(type);
}

inline
const BasicType& asBasicType(const Type& type)
{
    return static_cast<const BasicType&>(type);
}


} // namespace sbe2comms
