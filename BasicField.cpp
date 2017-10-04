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

#include "BasicField.h"

#include <iostream>
#include <set>

#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"
#include "EnumType.h"

namespace ba = boost::algorithm;

namespace sbe2comms
{

namespace
{

bool writeBuiltInTypeInt(std::ostream& out, const std::string& type)
{
    static const std::set<std::string> Set = {
        "int8", "uint8", "int16", "uint16",
        "int32", "uint32", "int64", "uint64"
    };

    auto iter = Set.find(type);
    if (iter == Set.end()) {
        return false;
    }

    out << "sbe2comms::" << *iter << "<field::FieldBase>;\n\n";
    return true;
}

bool writeBuiltInTypeFloat(std::ostream& out, const std::string& type)
{
    static const std::set<std::string> Set = {
        "float", "double"
    };

    auto iter = Set.find(type);
    if (iter == Set.end()) {
        return false;
    }

    out << "sbe2comms::" << *iter << "Field<field::FieldBase>;\n\n";
    return true;
}

bool writeBuiltInType(std::ostream& out, const std::string& type)
{
    return writeBuiltInTypeInt(out, type) || writeBuiltInTypeFloat(out, type);
}

} // namespace

const std::string& BasicField::getType() const
{
    auto& p = getProps();
    assert(!p.empty());
    return prop::type(p);
}

const std::string& BasicField::getValueRef() const
{
    auto& p = getProps();
    assert(!p.empty());
    return prop::valueRef(p);
}

bool BasicField::parseImpl()
{
    do {
        if (isConstant()) {
            auto& valueRef = getValueRef();
            if (!valueRef.empty()) {
                m_type = getTypeFromValueRef();
                break;
            }
        }

        auto& typeName = getType();
        if (typeName.empty()) {
            log::error() << "The field \"" << getName() << "\" doesn't specify its type." << std::endl;
            return false;
        }

        m_type = getDb().findType(typeName);
        if (m_type != nullptr) {
            break;
        }

        m_type = getDb().getBuiltInType(typeName);
    } while (false);

    if (m_type == nullptr) {
        log::error() << "Unknown or invalid type for field \"" << getName() << "\"." << std::endl;
        return false;
    }

    if (!hasPresence()) {
        return true;
    }

    if (isRequired()) {
        return checkRequired();
    }

    if (isOptional()) {
        return checkOptional();
    }

    if (isConstant()) {
        return checkConstant();
    }

    log::error() << "Unknown presence token for field \"" << getName() << "\"." << std::endl;
    return false;
}

bool BasicField::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    if (!startWrite(out, db, indent)) {
        return false;
    }

    auto& p = props(db);
    auto& name = prop::name(p);
    assert(!name.empty());

    out << output::indent(indent) << "using " << name << " = ";

    auto& type = prop::type(p);
    if (type.empty()) {
        out << " ???;\n\n";
        std::cerr << output::indent(1) <<
            "ERROR: Field \"" << name << "\" does NOT specify type." << std::endl;
        return false;
    }

    auto& types = db.getTypes();
    auto typeIter = types.find(type);
    if (typeIter == types.end()) {
        bool builtIn = writeBuiltInType(out, type);
        if (builtIn) {
            return true;
        }

        out << " ???;\n\n";
        std::cerr << output::indent(1) <<
            "ERROR: Unknown type \"" << type << "\" for field \"" << name << "\"" << std::endl;
        return false;
    }

    // TODO: check presence

    assert(typeIter->second);
    typeIter->second->recordNormalUse();

    out << '\n' <<
        output::indent(indent + 1) << "field::" << type << "<\n" <<
        output::indent(indent + 2) << extraOptionsString(db) << '\n' <<
        output::indent(indent + 1) << ">;\n\n";
    return true;
}

bool BasicField::checkRequired() const
{
    assert(m_type != nullptr);
    if (!m_type->isRequired()) {
        log::error() << "Required field \"" << getName() << "\" references "
                        "optional/constant type " << m_type->getName() << "\"." << std::endl;
        return false;
    }

    return true;
}

bool BasicField::checkOptional() const
{
    assert(m_type != nullptr);
    if (m_type->isConstant()) {
        log::error() << "Optional field \"" << getName() << "\" references "
                        "constant type " << m_type->getName() << "\"." << std::endl;
        return false;
    }

    return true;
}

bool BasicField::checkConstant() const
{
    assert(m_type != nullptr);
    if (m_type->isConstant()) {
        return true;
    }

    if (m_type->kind() != Type::Kind::Enum) {
        log::error() << "Constant field \"" << getName() << "\" can reference only const types or non-const enum." << std::endl;
        return false;
    }

    auto& valueRef = getValueRef();
    if (valueRef.empty()) {
        log::error() << "The constant field \"" << getName() << "\" must specify valueRef property." << std::endl;
        return false;
    }

    auto sep = ba::find_first(valueRef, ".");
    if (!sep) {
        log::error() << "Failed to split valueRef into <type.value> pair." << std::endl;
        return false;
    }

    std::string valueStr(sep.end(), valueRef.end());
    if (!static_cast<const EnumType*>(m_type)->hasValue(valueStr)) {
        log::error() << "The field \"" << getName() << "\" references invalid value \"" << valueRef << "\"." << std::endl;
        return false;
    }
    return true;
}

const Type* BasicField::getTypeFromValueRef() const
{
    auto& valueRef = getValueRef();
    assert(!valueRef.empty());

    auto sep = ba::find_first(valueRef, ".");
    if (!sep) {
        log::error() << "Failed to split valueRef into <type.value> pair." << std::endl;
        return nullptr;
    }

    std::string enumName(valueRef.begin(), sep.begin());
    auto* type = getDb().findType(enumName);
    if (type == nullptr) {
        log::error() << "Enum type \"" << enumName << "\" referenced by field \"" << getName() << "\" does not exist." << std::endl;
        return nullptr;
    }

    if (type->kind() != Type::Kind::Enum) {
        log::error() << "Type \"" << enumName << "\" rerences by field \"" << getName() << "\" is not an enum." << std::endl;
        return nullptr;
    }
    return type;
}

} // namespace sbe2comms
