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

#include "xml_wrap.h"

namespace sbe2comms
{

namespace prop
{

const std::string& name(const XmlPropsMap& map);
const std::string& type(const XmlPropsMap& map);
const std::string& description(const XmlPropsMap& map);
bool hasDeprecated(const XmlPropsMap& map);
unsigned deprecated(const XmlPropsMap& map);
unsigned version(const XmlPropsMap& map);
bool hasSinceVersion(const XmlPropsMap& map);
unsigned sinceVersion(const XmlPropsMap& map);
unsigned length(const XmlPropsMap& map);
unsigned offset(const XmlPropsMap& map);
unsigned blockLength(const XmlPropsMap& map);
const std::string& primitiveType(const XmlPropsMap& map);
const std::string& semanticType(const XmlPropsMap& map);
const std::string& byteOrder(const XmlPropsMap& map);
const std::string& headerType(const XmlPropsMap& map);
const std::string& presence(const XmlPropsMap& map);
bool isRequired(const XmlPropsMap& map);
bool isConstant(const XmlPropsMap& map);
bool isOptional(const XmlPropsMap& map);
bool hasMinValue(const XmlPropsMap& map);
bool hasMaxValue(const XmlPropsMap& map);
const std::string& minValue(const XmlPropsMap& map);
const std::string& maxValue(const XmlPropsMap& map);
const std::string& nullValue(const XmlPropsMap& map);
const std::string& encodingType(const XmlPropsMap& map);
const std::string& characterEncoding(const XmlPropsMap& map);
const std::string& valueRef(const XmlPropsMap& map);
const std::string& dimensionType(const XmlPropsMap& map);
unsigned id(const XmlPropsMap& map);

} // namespace prop

} // namespace sbe2comms
