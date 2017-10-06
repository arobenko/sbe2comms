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
        const std::string& msgName)
      : m_db(db),
        m_node(node),
        m_msgName(msgName)
    {
    }

    virtual ~Field() noexcept {}

    bool parse();

    bool doesExist() const;

    const std::string& getName() const;

    const std::string& getDescription() const;

    static Ptr create(DB& db, xmlNodePtr node, const std::string& msgName);

    bool write(std::ostream& out, DB& db, unsigned indent = 0);

    const XmlPropsMap& props(DB& db);

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
    void updateExtraHeaders(std::set<std::string>& headers);

protected:
    virtual bool parseImpl();
    virtual bool writeImpl(std::ostream& out, DB& db, unsigned indent) = 0;

    bool startWrite(std::ostream& out, DB& db, unsigned indent);
    bool writeBrief(std::ostream& out, unsigned indent, bool extraOpts = false);
    static void writeOptions(std::ostream& out, unsigned indent);
    void recordExtraHeader(const std::string& header);

    xmlNodePtr getNode() const
    {
        return m_node;
    }

    const std::string& getMsgName() const
    {
        return m_msgName;
    }

    std::string extraOptionsString(DB& db);

    DB& getDb()
    {
        return m_db;
    }

    const DB& getDb() const
    {
        return m_db;
    }


private:
    DB& m_db;
    xmlNodePtr m_node = nullptr;
    const std::string& m_msgName;
    XmlPropsMap m_props;
    std::set<std::string> m_extraHeaders;
};

using FieldPtr = Field::Ptr;

} // namespace sbe2comms
