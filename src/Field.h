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
#include <iosfwd>
#include <set>

#include "xml_wrap.h"
#include "Type.h"

namespace sbe2comms
{

class DB;
class Field
{
public:
    using Ptr = std::unique_ptr<Field>;
    explicit Field(
        DB& db,
        xmlNodePtr node,
        const std::string& scope)
      : m_db(db),
        m_node(node),
        m_scope(scope)
    {
    }

    enum class Kind
    {
        Basic,
        Group,
        Data
    };

    virtual ~Field() noexcept {}

    bool parse();

    bool doesExist() const;

    const std::string& getName() const;

    const std::string& getReferenceName() const;

    const std::string& getDescription() const;

    static Ptr create(DB& db, xmlNodePtr node, const std::string& scope);

    bool write(std::ostream& out, unsigned indent = 0);

    const XmlPropsMap& getProps() const
    {
        return m_props;
    }

    bool hasPresence() const;
    bool isRequired() const;
    bool isOptional() const;
    bool isConstant() const;
    unsigned getDeprecated() const;
    unsigned getSinceVersion() const;
    const std::string& getType() const;
    unsigned getOffset() const;
    void updateExtraHeaders(std::set<std::string>& headers);
    bool isCommsOptionalWrapped() const;

    Kind getKind() const
    {
        return getKindImpl();
    }

    bool usesBuiltInType() const
    {
        return usesBuiltInTypeImpl();
    }

    bool writeDefaultOptions(std::ostream& out, unsigned indent, const std::string& scope)
    {
        return writeDefaultOptionsImpl(out, indent, scope);
    }

protected:
    virtual Kind getKindImpl() const = 0;
    virtual bool parseImpl();
    virtual bool writeImpl(std::ostream& out, unsigned indent, const std::string& suffix) = 0;
    virtual bool usesBuiltInTypeImpl() const = 0;
    virtual bool writeDefaultOptionsImpl(std::ostream& out, unsigned indent, const std::string& scope);

    void writeHeader(std::ostream& out, unsigned indent, const std::string& suffix);
    static void writeOptions(std::ostream& out, unsigned indent);
    void recordExtraHeader(const std::string& header);
    std::string getFieldOptString() const;
    std::string getTypeOptString(const Type& type) const;

    xmlNodePtr getNode() const
    {
        return m_node;
    }

    const std::string& getScope() const
    {
        return m_scope;
    }


    DB& getDb()
    {
        return m_db;
    }

    const DB& getDb() const
    {
        return m_db;
    }


private:
    const std::string& getDefaultOptMode() const;

    DB& m_db;
    xmlNodePtr m_node = nullptr;
    std::string m_scope;
    XmlPropsMap m_props;
    std::set<std::string> m_extraHeaders;
};

using FieldPtr = Field::Ptr;

} // namespace sbe2comms
