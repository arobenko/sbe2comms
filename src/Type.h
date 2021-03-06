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

#include <memory>
#include <array>
#include <iosfwd>
#include <cstdint>
#include <utility>
#include <set>
#include <vector>

#include "xml_wrap.h"
#include "prop.h"

namespace sbe2comms
{

class DB;
class Type
{
public:
    using Ptr = std::unique_ptr<Type>;
    using ExtraIncludes = std::set<std::string>;
    using ExtraOptInfo = std::pair<std::string, std::string>;
    using ExtraOptInfosList = std::list<ExtraOptInfo>;
    using AliasTemplateArgsList = std::vector<std::string>;

    enum class Kind
    {
        Basic,
        Composite,
        Enum,
        Set,
        Ref
    };

    Type(DB& db, xmlNodePtr node);
    virtual ~Type() noexcept;

    xmlNodePtr getNode() const
    {
        return m_node;
    }

    bool parse();
    bool doesExist();

    const char* getNodeName() const;

    const std::string& getName() const;
    const std::string& getReferenceName() const;
    const std::string& getDescription() const;
    bool isRequired() const;
    bool isOptional() const;
    bool isConstant() const;
    const std::string& getPresence() const;
    unsigned getLengthProp() const;
    unsigned getOffset() const;
    const std::string& getMinValue() const;
    const std::string& getMaxValue() const;
    const std::string& getNullValue() const;
    const std::string& getSemanticType() const;
    const std::string& getCharacterEncoding() const;
    const std::string& getEncodingType() const;
    unsigned getSinceVersion() const;
    void updateExtraIncludes(ExtraIncludes& extraIncludes);

    static Ptr create(DB& db, xmlNodePtr node);

    Kind getKind() const
    {
        return getKindImpl();
    }

    bool writeProtocolDef();
    bool write(std::ostream& out, unsigned indent = 0);
    bool writePluginProperties(std::ostream& out, unsigned indent, const std::string& scope = std::string());

    bool writeDefaultOptions(std::ostream& out, unsigned indent, const std::string& scope)
    {
        return writeDefaultOptionsImpl(out, indent, scope);
    }

    const XmlPropsMap& getProps() const
    {
        return m_props;
    }

    std::size_t getSerializationLength() const
    {
        return getSerializationLengthImpl();
    }

    bool hasFixedLength() const
    {
        return hasFixedLengthImpl();
    }

    ExtraOptInfosList getExtraOptInfos() const
    {
        return getExtraOptInfosImpl();
    }

    bool canBeExtendedAsOptional() const
    {
        if (isConstant()) {
            return false;
        }

        return canBeExtendedAsOptionalImpl();
    }

    template <typename T>
    void addExtraOption(T&& opt)
    {
        m_extraOptions.push_back(std::forward<T>(opt));
    }

    void setContainingCompositeVersion(unsigned version)
    {
        m_containingCompositeVersion = version;
    }

    void setForcedBigEndianBase()
    {
        m_forcedBigEndianBase = true;
    }
    
    bool isCommsOptionalWrapped() const;

    void updateNodeProperties();

    AliasTemplateArgsList getAliasTemplateArguments() const
    {
        return getAliasTemplateArgumentsImpl();
    }

    Type* getRealType()
    {
        return getRealTypeImpl();
    }

protected:

    DB& getDb()
    {
        return m_db;
    }

    const DB& getDb() const
    {
        return m_db;
    }

    const std::vector<std::string>& getExtraOptions() const
    {
        return m_extraOptions;
    }

    virtual Kind getKindImpl() const = 0;
    virtual bool parseImpl();
    virtual bool writeImpl(std::ostream& out, unsigned indent, bool commsOptionalWrapped) = 0;
    virtual bool writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope);
    virtual std::size_t getSerializationLengthImpl() const = 0;
    virtual bool hasFixedLengthImpl() const = 0;
    virtual ExtraOptInfosList getExtraOptInfosImpl() const;
    virtual bool canBeExtendedAsOptionalImpl() const;
    virtual bool writePluginPropertiesImpl(
        std::ostream& out,
        unsigned indent,
        const std::string& scope) = 0;
    virtual AliasTemplateArgsList getAliasTemplateArgumentsImpl() const;
    virtual Type* getRealTypeImpl();

    void writeBrief(std::ostream& out, unsigned indent, bool commsOptionalWrapped);
    void writeHeader(
        std::ostream& out,
        unsigned indent,
        bool commsOptionalWrapped,
        bool extraOpts = true);
    void writeElementBrief(std::ostream& out, unsigned indent);
    void writeElementHeader(std::ostream& out, unsigned indent);
    void writeExtraOptions(std::ostream& out, unsigned indent);
    std::string nodeText();
    void addExtraInclude(const std::string& val);
    static std::size_t primitiveLength(const std::string& type);
    static std::pair<std::intmax_t, bool> stringToInt(const std::string& str);
    static std::intmax_t builtInIntNullValue(const std::string& type);
    void scopeToPropertyDefNames(
        const std::string& scope,
        std::string* fieldType,
        std::string* propsName);
    static const std::string& getNameSuffix(bool commsOptionalWrapped, bool isElement);
    std::string getFieldBaseString() const;
    static void writeSerialisedHiddenCheck(std::ostream& out, unsigned indent, const std::string& prop);
private:
    const std::string& getDefaultOptMode() const;

    DB& m_db;
    xmlNodePtr m_node = nullptr;
    XmlPropsMap m_props;
    ExtraIncludes m_extraIncludes;
    std::vector<std::string> m_extraOptions;
    unsigned m_containingCompositeVersion = 0U;
    bool m_forcedBigEndianBase = false;
};

using TypePtr = Type::Ptr;

} // namespace sbe2comms
