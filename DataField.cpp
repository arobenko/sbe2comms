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

#include "DataField.h"

#include <iostream>

#include "DB.h"
#include "prop.h"
#include "output.h"
#include "log.h"

namespace sbe2comms
{

Field::Kind DataField::getKindImpl() const
{
    return Kind::Data;
}

bool DataField::parseImpl()
{
    auto& type = getType();
    if (type.empty()) {
        log::error() << "The field \"" << getName() << "\" doesn't specify its type." << std::endl;
        return false;
    }

    auto* typePtr = getDb().findType(type);
    if (typePtr == nullptr) {
        log::error() << "Type \"" << type << "\" required by field \"" << getName() << "\" hasn't been found." << std::endl;
        return false;
    }

    if (typePtr->kind() != Type::Kind::Composite) {
        log::error() << "Type \"" << type << "\" references by field \"" << getName() << "\" is expected to be composite." << std::endl;
        return false;
    }

    typePtr->recordDataUse();
    m_type = typePtr;
    return true;
}

bool DataField::writeImpl(std::ostream& out, unsigned indent, const std::string& suffix)
{
    assert(m_type != nullptr);
    assert(m_type->hasListOrString());
    writeBrief(out, indent, suffix, true);
    writeOptions(out, indent);
    out << output::indent(indent) << "using " << getName() << suffix << " = " <<
           "field::" << m_type->getReferenceName() << "<TOpt...>;\n\n";
    return true;
}

bool DataField::hasListOrStringImpl() const
{
    return true;
}

} // namespace sbe2comms
