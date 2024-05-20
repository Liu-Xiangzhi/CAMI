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

#ifndef CAMI_LIB_UTILS_H
#define CAMI_LIB_UTILS_H

#include <cstdint>
#include <lib/assert.h>
#include <cstring>
#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace cami::lib {

constexpr bool isTwosPower(uint64_t val)
{
    return (val & (val - 1)) == 0;
}

constexpr uint64_t log2(uint64_t val)
{
    ASSERT(isTwosPower(val) && val > 0, "this function only accepts positive values which are 2's power");
#if defined(__GNUC__) || defined(__clang__)
    return 63 - __builtin_clzll(val);
// `_BitScanReverse64` cannot be used under a constexpr context, so just give up optimize in Windows
//
// #elif defined(_MSC_VER)
//     unsigned long res;
//     _BitScanReverse64(&res, val);
//     return res - 1;
#else
    for (size_t i = 0; i < 64; ++i) {
        if (val & 1) {
            return i;
        }
        val >>= 1;
    }
    ASSERT(false, "cannot reach here");
    return 0;
#endif
}

constexpr uint64_t roundUpDiv(uint64_t a, uint64_t b)
{
    return a / b + (a % b != 0);
}

constexpr uint64_t roundUp(uint64_t value, uint64_t base)
{
    ASSERT(isTwosPower(base), "`base` must be the power of 2");
    return (value + base - 1) & ~(base - 1);
}

// roundUpPadding(v, b) == roundUp(v, b) - v
constexpr uint64_t roundUpPadding(uint64_t value, uint64_t base)
{
    ASSERT(isTwosPower(base), "`base` must be the power of 2");
    return (-value) & (base - 1);
}

constexpr uint64_t nthPower(uint64_t val, uint64_t n) // NOLINT
{
    if (n == 0) {
        return 1;
    }
    if (n == 1) {
        return val;
    }
    return nthPower(val * val, n / 2) * (n % 2 ? val : 1);
}

constexpr uint64_t roundUpNthRoot(uint64_t val, uint64_t n)
{
    ASSERT(n > 0, "0-th root is invalid");
    // TODO optimize this
    if (val == 0) {
        return 0;
    }
    if (n == 1) {
        return val;
    }
    uint64_t last_nth_power_value = 0;
    for (int i = 0; i < val; ++i) {
        auto nth_power_value = nthPower(i, n);
        if (nth_power_value >= val || nth_power_value < last_nth_power_value /* overflow */) {
            return i;
        }
        last_nth_power_value = nth_power_value;
    }
    ASSERT(false, "cannot reach here");
    return 0;
}

template<int byte_cnt>
void inline write(uint8_t* target, uint64_t val)
{
    if constexpr (byte_cnt > 8 || byte_cnt <= 0) {
        static_assert(!std::is_same_v<int, decltype(byte_cnt)>, "`byte_cnt` should in range [1, 7]");
    }
#ifdef CAMI_TARGET_INFO_LITTLE_ENDIAN
    std::memcpy(target, &val, byte_cnt);
#else
    for (int i = 0; i < byte_cnt; ++i) {
        target[i] = (val >> (8 * i)) & 0xff;
    }
#endif
}

template<int byte_cnt>
inline uint64_t readU(const uint8_t* source)
{
    if constexpr (byte_cnt > 8 || byte_cnt <= 0) {
        static_assert(!std::is_same_v<int, decltype(byte_cnt)>, "`byte_cnt` should in range [1, 8]");
    }
#ifdef CAMI_TARGET_INFO_LITTLE_ENDIAN
    uint64_t val = 0;
    std::memcpy(&val, source, byte_cnt);
#else
    for (int i = 0; i < byte_cnt; ++i) {
        val = (val << 8) | source[i];
    }
#endif
    return val;
}

template<int byte_cnt>
inline uint64_t readI(const uint8_t* source)
{
    auto val = static_cast<int64_t>(readU<byte_cnt>(source));
    return (val << (64 - byte_cnt * 8)) >> (64 - byte_cnt * 8);
}

inline namespace literals {
constexpr uint64_t operator ""_k(unsigned long long val)
{
    return val * 1024;
}

constexpr uint64_t operator ""_m(unsigned long long val)
{
    return operator ""_k(val) * 1024;
}

constexpr uint64_t operator ""_g(unsigned long long val)
{
    return operator ""_m(val) * 1024;
}

constexpr uint64_t operator ""_K(unsigned long long val)
{
    return operator ""_k(val);
}

constexpr uint64_t operator ""_M(unsigned long long val)
{
    return operator ""_m(val);
}

constexpr uint64_t operator ""_G(unsigned long long val)
{
    return operator ""_g(val);
}

} // namespace literals

} // namespace cami::lib

#endif //CAMI_LIB_UTILS_H
