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

#ifndef CAMI_AM_TRACE_H
#define CAMI_AM_TRACE_H

#include "trace_data.h"
#include "object.h"

namespace cami::am {

class AbstractMachine;

struct Trace
{
    static void attachTag(AbstractMachine& am, Object& object, const Object::Tag& tag);
    static void updateTag(AbstractMachine& am, Object& obj, const Object::Tag& tag);
    static bool isIndeterminatelySequenced(AbstractMachine& am, const Object::Tag& new_, const Object::Tag& old);
};

} // namespace cami::am

#endif //CAMI_AM_TRACE_H
