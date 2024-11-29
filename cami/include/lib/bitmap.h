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

#ifndef CAMI_LIB_BITMAP_H
#define CAMI_LIB_BITMAP_H

#include <cstdint>
#include <initializer_list>
#include <string>
#include <cstring>
#include <lib/assert.h>
#include <lib/utils.h>

namespace cami::lib {
template<uint64_t N>
class Bitmap
{
    uint8_t bits[roundUpDiv(N, 8)]{};

public:
    Bitmap() = default;

    template<typename T>
    constexpr Bitmap(std::initializer_list<T> list)
    {
        for (const auto item: list) {
            this->set(static_cast<uint64_t>(item));
        }
    }

public:
    constexpr void set(uint64_t idx)
    {
        using namespace std::string_literals;
        ASSERT(idx < N, "index out of boundary. idx: "s + std::to_string(idx) + ", len: "s + std::to_string(N));
        this->bits[idx / 8] |= 1 << (idx % 8);
    }

    constexpr void unset(uint64_t idx)
    {
        using namespace std::string_literals;
        ASSERT(idx < N, "index out of boundary. idx: "s + std::to_string(idx) + ", len: "s + std::to_string(N));
        this->bits[idx / 8] &= ~(1 << (idx % 8));
    }

    constexpr void reset()
    {
        std::memset(this->bits, 0, sizeof this->bits);
    }

    [[nodiscard]] constexpr bool test(uint64_t idx) const
    {
        using namespace std::string_literals;
        ASSERT(idx < N, "index out of boundary. idx: "s + std::to_string(idx) + ", len: "s + std::to_string(N));
        return this->bits[idx / 8] & (1 << (idx % 8));
    }
};

} // namespace cami::lib

#endif //CAMI_LIB_BITMAP_H
