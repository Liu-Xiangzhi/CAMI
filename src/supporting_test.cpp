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

#include <iostream>
#include <cstdint>
#include <random>

std::random_device seed;
std::default_random_engine engine(seed());
std::uniform_int_distribution<uint64_t> int_dis(0, UINT64_MAX);
std::uniform_real_distribution<float> flt_dis;
std::uniform_real_distribution<double> dbl_dis;

bool test_floatingTypeSize()
{
    return sizeof(float) == 4 && sizeof(double) == 8;
}

bool test_signessConversion()
{
    const auto test1 = [](uint64_t val) {
        auto val2 = static_cast<int64_t>(val);
        return static_cast<uint64_t>(val2) == val;
    };
    const auto test2 = [](uint64_t val) {
        auto val2 = *reinterpret_cast<int64_t*>(&val);
        return static_cast<uint64_t>(val2) == val;
    };
    const auto test3 = [](uint64_t val) {
        auto val2 = *reinterpret_cast<int64_t*>(&val);
        auto p1 = reinterpret_cast<char*>(&val);
        auto p2 = reinterpret_cast<char*>(&val2);
        for (int i = 0; i < 8; ++i) {
            if (p1[i] != p2[i]) {
                return false;
            }
        }
        return true;
    };
    for (int i = 0; i < 1024; ++i) {
        auto val = int_dis(engine);
        if (!test1(val)) {
            return false;
        }
        if (!test2(val)) {
            return false;
        }
        if (!test3(val)) {
            return false;
        }
    }
    return true;
}

bool test_negativeIntegerRightShift()
{
    const auto test1 = [](uint64_t val) {
        val |= 0x8000'0000'0000'0000ULL;
        auto val2 = *reinterpret_cast<int64_t*>(&val);
        int64_t res;
        for (int i = 0; i < 63; ++i) {
            res = val2 >> i;
            auto res2 = val >> i;
            for (int j = 0; j < 64 - i; ++j) {
                auto mask = 1ULL << j;
                if ((mask & res) ^ (mask & res2)) {
                    return false;
                }
            }
            for (int j = 0; j < i; ++j) {
                auto mask = 1ULL << (63 - j);
                if (!(mask & res)) {
                    return false;
                }
            }
        }
        return true;
    };
    for (int i = 0; i < 1024; ++i) {
        auto val = int_dis(engine);
        if (!test1(val)) {
            return false;
        }
    }
    return true;
}

bool test_nullptr()
{
    struct Foo
    {
    };
    return reinterpret_cast<void*>(0) == nullptr
           && reinterpret_cast<uint64_t>(nullptr) == 0
           && reinterpret_cast<Foo*>(0) == nullptr;
}

bool test_conversationBetweenIntegerAndPointer()
{
    struct Foo
    {
        int bar[1024];
    };
    const auto test1 = [](uint64_t val) {
        auto val2 = reinterpret_cast<Foo*>(val);
        return reinterpret_cast<Foo*>(val + 8 - 8) == val2;
    };
    const auto test2 = [](uint64_t val) {
        auto val2 = reinterpret_cast<Foo*>(val);
        return reinterpret_cast<uint64_t>(val2  + 8 - 8) == val;
    };
    for (int i = 0; i < 1024; ++i) {
        auto val = int_dis(engine);
        if (!test1(val)) {
            return false;
        }
        if (!test2(val)) {
            return false;
        }
    }
    return true;
}

int main()
{
    bool ok = true;
    if (!test_floatingTypeSize()) {
        std::cerr << "Not support: condition violation `sizeof(float)==4 && sizeof(double)==8`";
        ok = false;
    }
    if (!test_signessConversion()) {
        std::cerr
                << "Not support: unsigned integer cannot cast to its corresponding signed type without changing any bit";
        ok = false;
    }
    if (!test_negativeIntegerRightShift()) {
        std::cerr << "Not support: right shift for negative number does not perform as arithmetic right shift";
        ok = false;
    }
    if (!test_nullptr()) {
        std::cerr << "Not support: nullptr cannot be assumed as 0";
        ok = false;
    }
    if (!test_conversationBetweenIntegerAndPointer()) {
        std::cerr << "Not support: pointer/integer cannot cast to integer/pointer without changing any bit";
        ok = false;
    }
    if (ok) {
        std::cout << "OK\n";
    }
}
