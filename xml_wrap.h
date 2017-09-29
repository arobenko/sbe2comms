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
#include <string>
#include <map>
#include <list>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


namespace sbe2comms
{

struct XmlDocFree
{
    void operator()(::xmlDocPtr p) const
    {
        xmlFree(p);
    }
};

struct XmlNodeFree
{
    void operator()(::xmlNodePtr p) const
    {
        xmlFreeNode(p);
    }
};

struct XmlCharFree
{
    void operator()(xmlChar* p) const
    {
        xmlFree(p);
    }
};

using XmlDocPtr = std::unique_ptr<xmlDoc, XmlDocFree>;
using XmlCharPtr = std::unique_ptr<xmlChar, XmlCharFree>;
using XmlPropsMap = std::map<std::string, std::string>;
using XmlNodePtr = std::unique_ptr<xmlNode, XmlNodeFree>;

XmlPropsMap xmlParseNodeProps(xmlNodePtr node, xmlDocPtr doc);
std::string xmlText(xmlNodePtr node);
std::list<xmlNodePtr> xmlChildren(xmlNodePtr node, const std::string& name = std::string());
XmlNodePtr xmlCreatePadding(unsigned idx, unsigned len);

} // namespace sbe2comms
