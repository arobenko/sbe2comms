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

namespace sbe2comms
{

RefType::Kind RefType::kindImpl() const
{
    return Kind::Ref;
}

bool RefType::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    auto& ptr = getReferenceType(db);
    if (!ptr) {
        return false;
    }

    auto& p = props(db);
    auto& n = prop::name(p);
    if (n.empty()) {
        std::cerr << "ERROR: Unknown name of the \"ref\" element" << std::endl;
        return false;
    }

    auto& refProps = ptr->props(db);
    auto& refName = prop::name(refProps);
    if (refName.empty()) {
        std::cerr << "ERROR: Unknown reference type of \"" << n << "\" ref." << std::endl;
        return false;
    }

    writeBrief(out, db, indent);
    out << output::indent(indent) << "using " << n << " = field::" << refName << ";\n\n";
    return true;
}

std::size_t RefType::lengthImpl(DB& db)
{
    auto& ptr = getReferenceType(db);
    if (!ptr) {
        return 0U;
    }

    return ptr->length(db);
}

bool RefType::writeDependenciesImpl(std::ostream& out, DB& db, unsigned indent)
{
    auto& ptr = getReferenceType(db);
    if (!ptr) {
        return false;
    }

    return ptr->write(out, db, indent);
}

const RefType::Ptr& RefType::getReferenceType(DB& db)
{
    auto& p = props(db);
    auto& t = prop::type(p);

    static const Ptr NoRef;
    if (t.empty()) {
        std::cerr << "ERROR: Unknown reference type for ref \"" << prop::name(p) << "\"." << std::endl;
        return NoRef;
    }

    auto& types = db.getTypes();
    auto iter = types.find(t);
    if (iter == types.end()) {
        std::cerr << "ERROR: Unknown type \"" << t << "\" in ref \"" << prop::name(p) << "\"." << std::endl;
        return NoRef;
    }

    return iter->second;
}

} // namespace sbe2comms
