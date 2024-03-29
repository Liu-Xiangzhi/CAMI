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

#ifndef CAMI_CAMI_IO_DEF_H
#define CAMI_CAMI_IO_DEF_H

#define SUCCESS 0
#define E_SYSTEM (-1)
#define E_INVALID_ADDRESS (-2)
#define E_INVALID_FD (-3)
#define E_FD_EXHAUSTED (-4)
#define E_BAD_IN_BUF (-5)
#define E_BAD_OUT_BUF (-6)
#define E_DENY (-7)
#define E_INVALID_ANCHOR (-8)
#define E_NOT_EXIST (-9)
#define MODE_READ_MASK 1
#define MODE_WRITE_MASK 2
#define MODE_BMODE_MASK 3
#define MODE_READ_ONLY MODE_READ_MASK
#define MODE_WRITE_ONLY MODE_WRITE_MASK
#define MODE_READ_WRITE (MODE_READ_MASK | MODE_WRITE_MASK)
#define MODE_TEST 0
#define MODE_CREAT 100
#define MODE_TRUNC 8
#define SEEK_HEAD 0
#define SEEK_CURRENT 1
#define SEEK_TAIL 2

#endif //CAMI_CAMI_IO_DEF_H
