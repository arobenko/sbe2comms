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
    using Members = std::vector<TypePtr>;
    explicit CompositeType(DB& db, xmlNodePtr node) : Base(db, node) {}

    bool isBundleOptional() const;
    bool verifyValidDimensionType() const;
    bool isValidData() const;
    bool isBundle() const;
    void recordDataUse()
    {
        m_dataUse = true;
    }

    bool dataUseRecorded() const
    {
        return m_dataUse;
    }

    const Members& getMembers() const
    {
        return m_members;
    }

protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent) override;
    virtual bool writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope) override;
    virtual std::size_t getSerializationLengthImpl() const override;
    virtual bool writeDependenciesImpl(std::ostream& out, unsigned indent) override;
    virtual bool hasFixedLengthImpl() const override;
    virtual ExtraOptInfosList getExtraOptInfosImpl() const override;
private:
    using AllExtraOptInfos = std::vector<ExtraOptInfosList>;

    bool prepareMembers();
    bool writeMembers(std::ostream& out, unsigned indent);
    bool writeBundle(std::ostream& out, unsigned indent);
    bool writeData(std::ostream& out, unsigned indent);
    bool checkDataValid();
    AllExtraOptInfos getAllExtraOpts() const;
    void writeExtraOptsDoc(std::ostream& out, unsigned indent, const AllExtraOptInfos& infos);
    void writeExtraOptsTemplParams(
        std::ostream& out,
        unsigned indent,
        const AllExtraOptInfos& infos,
        bool hasExtraOptions = false);
    bool isMessageHeader() const;
    bool checkMessageHeader();

    Members m_members;
    bool m_dataUse = false;
};

inline
const CompositeType* asCompositeType(const Type* type)
{
    return static_cast<const CompositeType*>(type);
}

inline
CompositeType* asCompositeType(Type* type)
{
    return static_cast<CompositeType*>(type);
}

inline
const CompositeType& asCompositeType(const Type& type)
{
    return static_cast<const CompositeType&>(type);
}


} // namespace sbe2comms
