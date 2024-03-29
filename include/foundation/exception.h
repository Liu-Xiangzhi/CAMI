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

#ifndef CAMI_FOUNDATION_EXCEPTION_H
#define CAMI_FOUNDATION_EXCEPTION_H

#include <stdexcept>
#include <string>
#include <string_view>

namespace cami {
using std::operator ""s;

class FileCannotOpenException : public std::runtime_error
{
public:
    explicit FileCannotOpenException(std::string_view file_name)
            : std::runtime_error("cannot open file: "s.append(file_name)) {}
};

class CannotDetectFileTypeException : public std::runtime_error
{
public:
    explicit CannotDetectFileTypeException(std::string_view file_name)
            : std::runtime_error("cannot file type of "s.append(file_name)) {}
};

} // namespace cami

#endif //CAMI_FOUNDATION_EXCEPTION_H
