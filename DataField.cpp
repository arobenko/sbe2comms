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

namespace sbe2comms
{

bool DataField::writeImpl(std::ostream& out, DB& db, unsigned indent)
{
    static_cast<void>(out);
    static_cast<void>(db);
    static_cast<void>(indent);
    return true;
}

} // namespace sbe2comms
