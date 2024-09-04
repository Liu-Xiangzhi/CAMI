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

#ifndef CAMI_LIB_COMPILER_GUARANTEE_H
#define CAMI_LIB_COMPILER_GUARANTEE_H
#include <string>
#include <stdexcept>
#include <lib/format.h>
#include <config.h>
namespace cami {
class CompilerGuaranteeViolationException : public std::runtime_error
{
public:
    explicit CompilerGuaranteeViolationException(std::string_view expr, const char* file, size_t line,
                                                 const std::string& what) : runtime_error(lib::format(
            "Compiler guarantee `${}` check failed: ${}\n\tat ${}:${}", expr, what, file, line)) {}
};
} // namespace cami

#ifndef CAMI_DISABLE_COMPILER_GUARANTEE_CHECK
#define CAMI_NOEXCEPT
#define COMPILER_GUARANTEE(expr, what)                                      \
    do {                                                                    \
        if (!(expr)) {                                                      \
            throw ::cami::CompilerGuaranteeViolationException{              \
                #expr, __FILE__, __LINE__, what};                           \
        }                                                                   \
    } while(0)
#else
#define CAMI_NOEXCEPT noexcept
#define COMPILER_GUARANTEE(expr, what) static_cast<void>(0)
#endif

#endif //CAMI_LIB_COMPILER_GUARANTEE_H
