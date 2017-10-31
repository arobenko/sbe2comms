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
const std::string& builtinsDefFileName();
const std::string& defaultOptionsFileName();
const std::string& msgIdFileName();
const std::string& msgInterfaceFileName();
const std::string& allMessagesFileName();
const std::string& allMessagesStr();
const std::string& defaultOptionsStr();
const std::string& emptyString();
const std::string& renameKeyword(const std::string& value);
const std::string& extraOptionsDocStr();
const std::string& elementSuffixStr();
const std::string& extraOptionsTemplParamStr();
const std::string& fieldBaseStr();
const std::string& fieldBaseDefStr();
const std::string& messageBaseDefStr();
const std::string& enumValSuffixStr();
const std::string& enumNullValueStr();
const std::string& fieldNamespaceStr();
const std::string& fieldNamespaceNameStr();
const std::string& messageNamespaceStr();
const std::string& messageNamespaceNameStr();
const std::string& builtinNamespaceStr();
const std::string& memembersSuffixStr();
const std::string& fieldsSuffixStr();
const std::string& eqEmptyOptionStr();
const std::string& optParamPrefixStr();
const std::string& blockLengthStr();
const std::string& numInGroupStr();
const std::string& groupListStr();
const std::string& templateIdStr();
const std::string& schemaIdStr();
const std::string& versionStr();
const std::string& msgIdEnumName();
const std::string& messageHeaderLayerFileName();
const std::string& messageHeaderLayerStr();
const std::string& transportFrameFileName();
const std::string& messageHeaderFrameStr();

std::string num(std::intmax_t val);
std::string scopeFor(const std::string& ns, const std::string type);
std::string pathTo(const std::string& ns, const std::string path);
const std::string& primitiveTypeToStdInt(const std::string& type);

void writeDetails(std::ostream& out, unsigned indent, const std::string& desc);
void writeExtraOptionsDoc(std::ostream& out, unsigned indent);
void writeExtraOptionsTemplParam(std::ostream& out, unsigned indent);
void writeIntIsNullFunc(std::ostream& out, unsigned indent, intmax_t val);
void writeFpIsNullFunc(std::ostream& out, unsigned indent);
void writeFpOptConstructor(
    std::ostream& out,
    unsigned indent,
    const std::string& name,
    const std::string& customDefault = std::string());
void writeFpValidCheckFunc(std::ostream& out, unsigned indent, bool nanValid = false);
void writeEnumIsNullFunc(std::ostream& out, unsigned indent);
void writeProtocolNamespaceBegin(const std::string& ns, std::ostream& out);
void writeProtocolNamespaceEnd(const std::string& ns, std::ostream& out);
void recordExtraHeader(const std::string& newHeader, std::set<std::string>& allHeaders);
std::string protocolDirRelPath(const std::string& ns, const std::string& extraDir = std::string());
bool createProtocolDefDir(
    const std::string& root,
    const std::string& ns,
    const std::string& extraDir = std::string());

} // namespace common

} // namespace sbe2comms
