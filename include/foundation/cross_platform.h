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

#ifndef CAMI_FOUNDATION_CROSS_PLATFORM_H
#define CAMI_FOUNDATION_CROSS_PLATFORM_H

#ifdef CAMI_TARGET_INFO_UNIX_LIKE
#include <unistd.h>
#define FD int
#define IVD_FD (-1)
#define STDIN_FD STDIN_FILENO
#define STDOUT_FD STDOUT_FILENO
#define STDERR_FD STDERR_FILENO

#elif defined(CAMI_TARGET_INFO_WINDOWS)
#include <windows.h>
#define FD HANDLE
#define IVD_FD INVALID_HANDLE_VALUE
#define STDIN_FD GetStdHandle(STD_INPUT_HANDLE)
#define STDOUT_FD GetStdHandle(STD_OUTPUT_HANDLE)
#define STDERR_FD GetStdHandle(STD_ERROR_HANDLE)
#else
#error unsupported OS
#endif

#endif //CAMI_FOUNDATION_CROSS_PLATFORM_H
