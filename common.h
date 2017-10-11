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
#include <iosfwd>
#include <set>
#include <cstdint>

namespace sbe2comms
{

namespace common
{

const std::string& messageDirName();
const std::string& includeDirName();
const std::string& fieldsDefFileName();
const std::string& emptyString();
const std::string& renameKeyword(const std::string& value);
const std::string& extraOptionsDocStr();
const std::string& elementSuffixStr();
const std::string& extraOptionsTemplParamStr();
const std::string& fieldBaseStr();
const std::string& fieldBaseDefStr();
const std::string& enumValSuffixStr();
const std::string& enumNullValueStr();
std::string num(std::intmax_t val);

void writeDetails(std::ostream& out, unsigned indent, const std::string& desc);
void writeExtraOptionsDoc(std::ostream& out, unsigned indent);
void writeExtraOptionsTemplParam(std::ostream& out, unsigned indent);
void writeIntIsNullFunc(std::ostream& out, unsigned indent, intmax_t val);
void writeFpIsNullFunc(std::ostream& out, unsigned indent);
void writeEnumIsNullFunc(std::ostream& out, unsigned indent);
void recordExtraHeader(const std::string& newHeader, std::set<std::string>& allHeaders);

} // namespace common

} // namespace sbe2comms
