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

#include <list>
#include <vector>
#include "Field.h"
#include "Type.h"

namespace sbe2comms
{

class GroupField : public Field
{
    using Base = Field;
public:
    GroupField(DB& db, xmlNodePtr node, const std::string& msgName)
      : Base(db, node, msgName)
    {
    }

protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent, const std::string& suffix) override;
    virtual bool usesBuiltInTypeImpl() const override;
    virtual bool writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope) override;

private:
    bool prepareMembers();
    unsigned getBlockLength() const;
    bool writeMembers(std::ostream& out, unsigned indent);
    void writeBundle(std::ostream& out, unsigned indent);
    void writeVersions(std::ostream& out, unsigned indent);
    const std::string& getDimensionType() const;
    bool writeMembersDefaultOptions(std::ostream& out, unsigned indent, const std::string& scope);

    std::list<FieldPtr> m_fields;
    std::vector<FieldPtr> m_members;
    const Type* m_type = nullptr;
};

} // namespace sbe2comms
