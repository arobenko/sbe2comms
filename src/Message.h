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

    bool write();
    bool writeDefaultOptions(std::ostream& out, unsigned indent, const std::string& scope);
    const std::string& getName() const;
    const std::string& getReferenceName() const;
    unsigned getId() const;
    bool hasFields() const;

private:

    bool createFields();
    bool writeFields(std::ostream& out);
    bool writeAllFieldsDef(std::ostream& out);
    bool writeMessageClass(std::ostream& out);
    void writeFieldsAccess(std::ostream& out) const;
    bool writeMessageDef(const std::string& filename);
    void writeConstructors(std::ostream& out);
    void writeReadFunc(std::ostream& out);
    void writeRefreshFunc(std::ostream& out);
    void writePrivateMembers(std::ostream& out);
    void writeExtraDefHeaders(std::ostream& out);
    bool writeProtocolDef();
    bool writePluginHeader();
    bool writePluginSrc();

    DB& m_db;
    xmlNodePtr m_node = nullptr;
    XmlPropsMap m_props;
    FieldsList m_fields;
};

} // namespace sbe2comms
