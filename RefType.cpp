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

#include "RefType.h"

#include <iostream>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"

namespace sbe2comms
{

RefType::Kind RefType::kindImpl() const
{
    return Kind::Ref;
}

bool RefType::parseImpl()
{
    if (!getReferenceType()) {
        return false;
    }

    return true;
}

bool RefType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    static_cast<void>(db);
    auto& ptr = getReferenceType();
    assert(ptr);
    assert(ptr->isWritten());
    auto& name = getName();
    assert(!name.empty());

    auto& refName = ptr->getName();
    assert(!refName.empty());

    writeBrief(out, db, indent);
    out << output::indent(indent) << "using " << name << " = field::" << refName << ";\n\n";
    return true;
}

std::size_t RefType::lengthImpl(DB& db)
{
    auto& ptr = getReferenceType();
    assert(ptr);
    return ptr->length(db);
}

bool RefType::writeDependenciesImpl(std::ostream& out, DB& db, unsigned indent)
{
    auto& ptr = getReferenceType();
    assert(ptr);
    return ptr->write(out, db, indent);
}

const RefType::Ptr& RefType::getReferenceType() const
{
    auto& p = getProps();
    auto& t = prop::type(p);

    static const Ptr NoRef;
    if (t.empty()) {
        log::error() << "Unknown reference type for ref \"" << getName() << "\"." << std::endl;
        return NoRef;
    }

    auto& types = getDb().getTypes();
    auto iter = types.find(t);
    if (iter == types.end()) {
        log::error() << "Unknown type \"" << t << "\" in ref \"" << getName() << "\"." << std::endl;
        return NoRef;
    }

    return iter->second;
}

} // namespace sbe2comms
