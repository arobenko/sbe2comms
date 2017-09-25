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

protected:
    virtual Kind kindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, DB& db, unsigned indent) override;
    virtual std::size_t lengthImpl(DB& db) override;

private:
    bool writeSimpleType(
        std::ostream& out,
        unsigned indent,
        bool embedded = false);

    bool writeSimpleInt(
        std::ostream& out,
        unsigned indent,
        const std::string& intType,
        bool embedded);

    bool writeSimpleFloat(
        std::ostream& out,
        unsigned indent,
        const std::string& fpType,
        bool embedded);

    bool writeVarLength(
        std::ostream& out,
        DB& db,
        unsigned indent,
        const std::string& primType);

    bool writeVarLengthString(
        std::ostream& out,
        DB& db,
        unsigned indent);

    bool writeVarLengthArray(
        std::ostream& out,
        DB& db,
        unsigned indent,
        const std::string& primType);

    bool writeVarLengthRawDataArray(
        std::ostream& out,
        DB& db,
        unsigned indent,
        const std::string& primType);

    bool writeFixedLength(
        std::ostream& out,
        DB& db,
        unsigned indent,
        const std::string& primType);

    bool writeFixedLengthString(
        std::ostream& out,
        DB& db,
        unsigned indent);

    bool writeFixedLengthArray(
        std::ostream& out,
        DB& db,
        unsigned indent,
        const std::string& primType);

    bool writeFixedLengthRawDataArray(
        std::ostream& out,
        DB& db,
        unsigned indent,
        const std::string& primType);

    bool hasMinMaxValues(DB& db);

    bool isString(DB& db);

    bool isConstString(DB& db);
};

} // namespace sbe2comms
