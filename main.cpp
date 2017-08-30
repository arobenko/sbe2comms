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

#include <iostream>
#include "DB.h"

int main(int argc, const char* argv[])
{

    if (argc < 2) {
        std::cerr << "Wrong number of arguments" << std::endl;
        return -1;
    }

    sbe2comms::DB db;
    if (!sbe2comms::parseSchema(argv[1], db)) {
        return -1;
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
