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

#include "MessageSchema.h"
#include "prop.h"

namespace sbe2comms
{

MessageSchema::MessageSchema(xmlNodePtr node, xmlDocPtr doc)
  : m_props(xmlParseNodeProps(node, doc))
{
}

const std::string& MessageSchema::package()
{
    return m_props["package"]; // may create missing node
}

unsigned MessageSchema::version() const
{
    return prop::version(m_props);
}

unsigned MessageSchema::id() const
{
    return prop::id(m_props);
}

const std::string& MessageSchema::byteOrder() const
{
    return prop::byteOrder(m_props);
}

const std::string& MessageSchema::headerType() const
{
    return prop::headerType(m_props);
}

} // namespace sbe2comms