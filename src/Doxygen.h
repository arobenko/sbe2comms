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

namespace sbe2comms
{

class DB;
class Doxygen
{
public:
    Doxygen(DB& db);
    bool write();

private:
    bool writeLayout();
    bool writeConf();
    bool writeNamespaces();
    bool writeMain();

    DB& m_db;
    std::string m_name;
};

} // namespace sbe2comms