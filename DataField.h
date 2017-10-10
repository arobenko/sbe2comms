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

#include "Field.h"
#include "Type.h"

namespace sbe2comms
{

class DataField : public Field
{
    using Base = Field;
public:
    DataField(DB& db, xmlNodePtr node, const std::string& msgName) : Base(db, node, msgName) {}

protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent, const std::string& suffix) override;
    virtual bool hasListOrStringImpl() const;

private:
    const Type* m_type = nullptr;
};

} // namespace sbe2comms
