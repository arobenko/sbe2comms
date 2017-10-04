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
#include <limits>

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
const std::string Deprecated("deprecated");
const std::string Version("version");
const std::string SinceVersion("sinceVersion");
const std::string Length("length");
const std::string Offset("offset");
const std::string PrimitiveType("primitiveType");
const std::string SemanticType("semanticType");
const std::string ByteOrder("byteOrder");
const std::string Presence("presence");
const std::string MinValue("minValue");
const std::string MaxValue("maxValue");
const std::string NullValue("nullValue");
const std::string EncodingType("encodingType");
const std::string CharacterEncoding("characterEncoding");
const std::string ValueRef("valueRef");
const std::string FailInvalid("cc_fail_invalid");

const std::string& getProp(const XmlPropsMap& map, const std::string propName)
{
    auto iter = map.find(propName);
    if (iter == map.end()) {
        return EmptyStr;
    }

    return iter->second;
}

bool hasProp(const XmlPropsMap& map, const std::string propName)
{
    return map.find(propName) != map.end();
}

template <typename T>
T getPropInt(
    const XmlPropsMap& map,
    const std::string propName,
    const T& defValue = T())
{
    try {
        auto strVal = getProp(map, propName);
        return static_cast<T>(std::stoll(strVal));
    }
    catch (...) {
        return defValue;
    }
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

bool hasDeprecated(const XmlPropsMap& map)
{
    return hasProp(map, Deprecated);
}

unsigned deprecated(const XmlPropsMap& map)
{
    return getPropInt<unsigned>(map, Deprecated, std::numeric_limits<unsigned>::max());
}

unsigned version(const XmlPropsMap& map)
{
    return getPropInt<unsigned>(map, Version);
}

bool hasSinceVersion(const XmlPropsMap& map)
{
    return hasProp(map, SinceVersion);
}

unsigned sinceVersion(const XmlPropsMap& map)
{
    return getPropInt<unsigned>(map, SinceVersion);
}

unsigned length(const XmlPropsMap& map)
{
    return getPropInt<unsigned>(map, Length, 1U);
}

unsigned offset(const XmlPropsMap& map)
{
    return getPropInt<unsigned>(map, Offset);
}

const std::string& primitiveType(const XmlPropsMap& map)
{
    return getProp(map, PrimitiveType);
}

const std::string& semanticType(const XmlPropsMap& map)
{
    return getProp(map, SemanticType);
}

const std::string& byteOrder(const XmlPropsMap& map)
{
    return getProp(map, ByteOrder);
}

const std::string& presence(const XmlPropsMap& map)
{
    return getProp(map, Presence);
}

bool isRequired(const XmlPropsMap& map)
{
    static const std::string RequiredStr("required");
    auto& val = getProp(map, Presence);
    return val.empty() || (val == RequiredStr);
}

bool isConstant(const XmlPropsMap& map)
{
    static const std::string ConstantStr("constant");
    return (getProp(map, Presence) == ConstantStr);
}

bool isOptional(const XmlPropsMap& map)
{
    static const std::string OptionalStr("optional");
    return (getProp(map, Presence) == OptionalStr);
}

bool hasMinValue(const XmlPropsMap& map)
{
    return hasProp(map, MinValue);
}

bool hasMaxValue(const XmlPropsMap& map)
{
    return hasProp(map, MaxValue);
}

const std::string& minValue(const XmlPropsMap& map)
{
    return getProp(map, MinValue);
}

const std::string& maxValue(const XmlPropsMap& map)
{
    return getProp(map, MaxValue);
}

const std::string& nullValue(const XmlPropsMap& map)
{
    return getProp(map, NullValue);
}

const std::string& encodingType(const XmlPropsMap& map)
{
    return getProp(map, EncodingType);
}

const std::string& characterEncoding(const XmlPropsMap& map)
{
    return getProp(map, CharacterEncoding);
}

const std::string& valueRef(const XmlPropsMap& map)
{
    return getProp(map, ValueRef);
}

const std::string& ccFailInvalid(const XmlPropsMap& map)
{
    return getProp(map, FailInvalid);
}

} // namespace prop

} // namespace sbe2comms
