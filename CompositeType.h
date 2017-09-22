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

#include <vector>

#include "Type.h"

namespace sbe2comms
{

class CompositeType : public Type
{
    using Base = Type;
public:
    explicit CompositeType(DB& db, xmlNodePtr node) : Base(db, node) {}

protected:
    virtual Kind kindImpl() const override;
    virtual bool writeImpl(std::ostream& out, DB& db, unsigned indent) override;
    virtual std::size_t lengthImpl(DB& db) override;
private:
    bool prepareMembers(DB& db);
    bool writeMembers(std::ostream& out, DB& db, unsigned indent);
    bool writeBundle(std::ostream& out, DB& db, unsigned indent);
    bool writeString(std::ostream& out, DB& db, unsigned indent);
    bool mustBeString();

    std::vector<TypePtr> m_members;
};

} // namespace sbe2comms
