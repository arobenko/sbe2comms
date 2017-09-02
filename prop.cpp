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

#include "prop.h"

#include <string>

namespace sbe2comms
{

namespace prop
{

namespace
{

const std::string EmptyStr;
const std::string Name("name");
const std::string Type("type");
const std::string Description("description");

const std::string& getProp(const XmlPropsMap& map, const std::string propName)
{
    auto iter = map.find(propName);
    if (iter == map.end()) {
        return EmptyStr;
    }

    return iter->second;
}

} // namespace

const std::string& name(const XmlPropsMap& map)
{
    return getProp(map, Name);
}

const std::string& type(const XmlPropsMap& map)
{
    return getProp(map, Type);
}

const std::string& description(const XmlPropsMap& map)
{
    return getProp(map, Description);
}

} // namespace prop

} // namespace sbe2comms
