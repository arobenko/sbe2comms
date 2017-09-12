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

#include "xml_wrap.h"

namespace sbe2comms
{

class DB;
class Field
{
public:
    using Ptr = std::unique_ptr<Field>;
    explicit Field(
        xmlNodePtr node,
        const std::string& msgName)
      : m_node(node),
        m_msgName(msgName)
    {
    }

    virtual ~Field() noexcept {}

    static Ptr create(xmlNodePtr node, const std::string& msgName);

    bool write(std::ostream& out, DB& db, unsigned indent = 0);

    const XmlPropsMap& props(DB& db);

protected:
    virtual bool writeImpl(std::ostream& out, DB& db, unsigned indent) = 0;

    bool startWrite(std::ostream& out, DB& db, unsigned indent);

    xmlNodePtr getNode() const
    {
        return m_node;
    }

    const std::string& getMsgName() const
    {
        return m_msgName;
    }

    std::string extraOptionsString(DB& db);
private:

    xmlNodePtr m_node = nullptr;
    const std::string& m_msgName;
    XmlPropsMap m_props;
};

using FieldPtr = Field::Ptr;

} // namespace sbe2comms
