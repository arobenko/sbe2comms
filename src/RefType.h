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

class RefType : public Type
{
    using Base = Type;
public:
    explicit RefType(DB& db, xmlNodePtr node) : Base(db, node) {}

    bool isReferredOptional() const;
protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent, bool commsOptionalWrapped) override;
    virtual bool writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope) override;
    virtual std::size_t getSerializationLengthImpl() const override;
    virtual bool hasFixedLengthImpl() const override;
    virtual ExtraOptInfosList getExtraOptInfosImpl() const override;
    virtual bool writePluginPropertiesImpl(
        std::ostream& out,
        unsigned indent,
        const std::string& scope) override;

private:
    Type* getReferenceType();
    bool isBundle() const;
    void writeBundle(std::ostream& out, unsigned indent, bool commsOptionalWrapped);
    const std::string& getTypeRefSuffix(bool commsOptionalWrapped);

    Type* m_type = nullptr;
};

inline
const RefType& asRefType(const Type& type)
{
    return static_cast<const RefType&>(type);
}

} // namespace sbe2comms
