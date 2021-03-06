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

class BasicField : public Field
{
    using Base = Field;
public:
    BasicField(DB& db, xmlNodePtr node, const std::string& scope) : Base(db, node, scope) {}

    const std::string& getValueRef() const;
    unsigned getSerializationLength() const;
    void setGeneratedPadding()
    {
        m_generatedPadding = true;
    }

protected:
    virtual Kind getKindImpl() const override;
    virtual unsigned getReferencedTypeSinceVersionImpl() const override;
    virtual bool isForcedCommsOptionalImpl() const override;
    virtual bool parseImpl() override;
    virtual bool writeImpl(std::ostream& out, unsigned indent, const std::string& suffix) override;
    virtual bool usesBuiltInTypeImpl() const override;
    virtual bool writePluginPropertiesImpl(
        std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool returnResult,
        bool commsOptionalWrapped) override;

private:
    bool checkRequired() const;
    bool checkOptional() const;
    bool checkConstant() const;
    const Type* getTypeFromValueRef() const;
    bool isSimpleAlias() const;
    void writePaddingAlias(std::ostream& out, unsigned indent, const std::string& name);
    void writeCompositeAlias(std::ostream& out, unsigned indent, const std::string& name);
    void writeSimpleAlias(std::ostream& out, unsigned indent, const std::string& name);
    void writeConstant(std::ostream& out, unsigned indent, const std::string& name);
    void writeOptional(std::ostream& out, unsigned indent, const std::string& name);
    void writeOptionalBasic(std::ostream& out, unsigned indent, const std::string& name);
    void writeOptionalBasicBigUnsignedInt(std::ostream& out, unsigned indent, const std::string& name);
    void writeOptionalBasicInt(std::ostream& out, unsigned indent, const std::string& name);
    void writeOptionalBasicFp(std::ostream& out, unsigned indent, const std::string& name);
    void writeOptionalEnum(std::ostream& out, unsigned indent, const std::string& name);
    bool writeBuiltinPluginProperties(
        std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool returnResult,
        bool commsOptionalWrapped);
    bool writeSimpleAliasPluginProperties(
        std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool returnResult,
        bool commsOptionalWrapped);
    bool writeConstantPluginProperties(
        std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool returnResult,
        bool commsOptionalWrapped);
    bool writeOptionalPluginProperties(
        std::ostream& out,
        unsigned indent,
        const std::string& scope,
        bool returnResult,
        bool commsOptionalWrapped);

    const Type* m_type = nullptr;
    bool m_generatedPadding = false;
};

} // namespace sbe2comms
