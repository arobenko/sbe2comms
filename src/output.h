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

#include <iosfwd>

namespace sbe2comms
{

namespace output
{

class Indent
{
public:
    Indent(unsigned count) : m_count(count) {}
    unsigned value() const { return m_count; }
private:
    unsigned m_count = 0U;
};

inline
Indent indent(unsigned count)
{
    return Indent(count);
}

} // namespace output

} // namespace sbe2comms


std::ostream& operator<<(std::ostream& out, sbe2comms::output::Indent i);
