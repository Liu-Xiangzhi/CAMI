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

#ifndef CAMI_CAMI_CAMISTD_H
#define CAMI_CAMI_CAMISTD_H

#ifdef __CAMIC__
#include "io_def.h"
#define malloc(num, type) __builtin new type[num]
#else
#define malloc(num, type) malloc((num)*sizeof(type))
#endif // __CAMIC__

#endif //CAMI_CAMI_CAMISTD_H
