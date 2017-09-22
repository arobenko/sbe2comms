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

#include <string>
#include <vector>
#include <iosfwd>

#include <boost/optional.hpp>

#include "xml_wrap.h"
#include "Field.h"

namespace sbe2comms
{

class DB;
class Message
{
public:
    using FieldsList = std::vector<FieldPtr>;

    Message(DB& db, xmlNodePtr node);
    Message(const Message&) = default;
    Message(Message&&) = default;

    bool parse();

    bool write(DB& db);
    const std::string& name(DB& db);

private:

    bool createFields(DB& db);
    bool insertField(FieldPtr field, DB& db);
    bool writeFields(std::ostream& out, DB& db);
    bool writeAllFieldsDef(std::ostream& out, DB& db);
    bool writeMessageClass(std::ostream& out, DB& db);
    void retrieveProps(DB& db);
    bool writeMessageDef(const std::string& filename, DB& db);

    DB& m_db;
    xmlNodePtr m_node = nullptr;
    XmlPropsMap m_props;
    boost::optional<FieldsList> m_fields;
};

} // namespace sbe2comms
