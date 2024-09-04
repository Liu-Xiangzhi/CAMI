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

#ifndef CAMI_FOUNDATION_ASSERT_H
#define CAMI_FOUNDATION_ASSERT_H

#ifdef NDEBUG
#define ASSERT(val, msg) static_cast<void>(0)
#else
#include <cstdlib>
#include <iostream>
#include <string>
namespace cami::detail {
[[noreturn]] inline void reportAssertionFail(const char* expr, const std::string& msg,
                                             const char* file, const char* func, int line)
{
    std::cerr << "\033[31mAssertion `" << expr << "` failed.\nIn " << file << ':' << line << '@' << func
              << "\n\treason: " << msg << "\033[0m\n";
    std::abort();
}
} // namespace cami::detail

#ifdef __GNUC__
#define CAMI_ASSERT_FAILED_FUNC __PRETTY_FUNCTION__
#else
#define CAMI_ASSERT_FAILED_FUNC  __func__
#endif
#define ASSERT(expr, msg)                                                                                  \
    do {                                                                                                   \
        if (!(expr)) {                                                                                     \
             ::cami::detail::reportAssertionFail(#expr, msg, __FILE__, CAMI_ASSERT_FAILED_FUNC, __LINE__); \
        }                                                                                                  \
    } while(0)
#endif

#endif //CAMI_FOUNDATION_ASSERT_H
