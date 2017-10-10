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

#include "get.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace get
{

const std::string& messageDirName()
{
    static const std::string Name("message");
    return Name;
}

const std::string& includeDirName()
{
    static const std::string Name("include");
    return Name;
}

const std::string& fieldsDefFileName()
{
    static const std::string Name("field.h");
    return Name;
}

const std::string& emptyString()
{
    static const std::string Str;
    return Str;
}

} // namespace get

} // namespace sbe2comms
