/*******************************************************************************
 * Copyright (c) 2024. Liu Xiangzhi
 * This file is part of CAMI.
 *
 * CAMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or any later version.
 *
 * CAMI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with CAMI.
 * If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef CAMI_TRANSLATE_DUMPER_H
#define CAMI_TRANSLATE_DUMPER_H

#include <iostream>
#include "bytecode.h"

namespace cami::tr {

// BBC ==> binary
class Dumper
{
public:
    void dump(const BBC& bbc, std::ostream& os);
};

} // namespace cami::tr

#endif //CAMI_TRANSLATE_DUMPER_H
