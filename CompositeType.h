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

    bool isBundleOptional() const;
    bool isValidDimensionType() const;
    bool isValidData() const;
    bool isBundle() const;

protected:
    virtual Kind getKindImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent) override;
    virtual bool writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope) override;
    virtual std::size_t getSerializationLengthImpl() const override;
    virtual bool writeDependenciesImpl(std::ostream& out, unsigned indent) override;
    virtual bool hasListOrStringImpl() const override;
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
    void writeExtraOptsTemplParams(std::ostream& out, unsigned indent, const AllExtraOptInfos& infos);

    std::vector<TypePtr> m_members;
};

} // namespace sbe2comms
