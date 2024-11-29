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

#ifndef CAMI_LIB_FORMAT_H
#define CAMI_LIB_FORMAT_H

#include <string_view>
#include <string>
#include <cstdint>
#include <initializer_list>
#include <lib/assert.h>

namespace cami::lib {

namespace detail {

struct FormatArg
{
    template<typename T>
    using arg_t = std::remove_cv_t<std::remove_reference_t<T>>;
    using f_t = std::string (*)(const void*, std::string_view);
    const void* arg;
    f_t to_string;
};

template<size_t size>
struct FormatArgs
{
    FormatArg args[size];

    FormatArgs(std::initializer_list<FormatArg> list) // NOLINT
    {
        auto itr = list.begin();
        for (size_t i = 0; i < size; ++i) {
            args[i] = *itr++;
        }
    }

    [[nodiscard]] const FormatArg* data() const noexcept
    {
        return this->args;
    }
};

[[nodiscard]] constexpr size_t getArgumentNum(std::string_view sv)
{
    size_t cnt = 0;
    for (size_t i = 0; i < sv.length(); ++i) {
        if (sv[i] == '$') {
            ASSERT(i + 1 < sv.length(), "wrong format syntax: trailing '$'");
            if (sv[i + 1] == '$') {
                i++;
                continue;
            }
            if (sv[i + 1] == '{') {
                cnt++;
                i++;
                size_t nest_cnt = 1;
                while (nest_cnt > 0) {
                    i++;
                    ASSERT(i < sv.length(), "wrong format syntax: brace mismatch");
                    if (sv[i] == '{') {
                        ++nest_cnt;
                    } else if (sv[i] == '}') {
                        --nest_cnt;
                    }
                }
            }
        }
    }
    return cnt;
}

inline std::string format(std::string_view format_string, const FormatArg* args)
{
    std::string result;
    size_t begin = 0;
    size_t arg_cnt = 0;
    // syntax should be checked in `getArgumentNum`
    for (size_t i = 0; i < format_string.length(); ++i) {
        if (format_string[i] == '$') {
            if (format_string[i + 1] == '$') {
                ++i;
                result.append(format_string.substr(begin, i - begin));
                begin = i + 1;
                continue;
            }
            result.append(format_string.substr(begin, i - begin));
            i++;
            const size_t specifier_begin = i + 1;
            size_t nest_cnt = 1;
            while (nest_cnt > 0) {
                i++;
                if (format_string[i] == '{') {
                    ++nest_cnt;
                } else if (format_string[i] == '}') {
                    --nest_cnt;
                }
            }
            auto specifier = format_string.substr(specifier_begin, i - specifier_begin);
            result.append(args[arg_cnt].to_string(args[arg_cnt].arg, specifier));
            arg_cnt++;
            begin = i + 1;
        }
    }
    result.append(format_string.substr(begin));
    return result;
}

inline std::string toHex(uint64_t val, size_t len)
{
    auto const toChar = [](uint8_t hex) {
        return hex < 10 ? hex + '0' : hex - 10 + 'A';
    };
    std::string result{"0x"};
    val <<= (8 - len) * 8;
    for (size_t i = 0; i < len; ++i) {
        auto byte = val >> 56;
        result.append(1, static_cast<char>(toChar(byte >> 4)));
        result.append(1, static_cast<char>(toChar(byte & 0xf)));
        val <<= 8;
    }
    return result;
}

inline std::string toBin(uint64_t val, size_t len)
{
    std::string result{"0b"};
    val <<= (8 - len) * 8;
    for (size_t i = 0; i < len; ++i) {
        auto byte = val >> 56;
        auto mask = 0x80;
        for (int j = 0; j < 8; ++j) {
            result.append(1, mask & byte ? '1' : '0');
            mask >>= 1;
        }
        val <<= 8;
    }
    return result;
}

template<typename T>
struct DefaultToString
{
    static std::string invoke(const T&, std::string_view)
    {
        static_assert(!std::is_same_v<T, T>, "not supported format type");
        return "";
    }
};

#ifndef NDEBUG
#define UNKNOWN_SPECIFIER() ASSERT(false, "unknown specifier")
#else
#define UNKNOWN_SPECIFIER() return ""
#endif

template<>
struct DefaultToString<bool>
{
    static std::string invoke(const bool& arg, [[maybe_unused]] std::string_view specifier)
    {
        return arg ? "true" : "false";
    }
};

template<>
struct DefaultToString<char>
{
    static std::string invoke(const char& arg, [[maybe_unused]] std::string_view specifier)
    {
        return {1, arg};
    }
};

template<>
struct DefaultToString<signed char>
{
    static std::string invoke(const signed char& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, 1);
        }
        if (specifier == "b") {
            return toBin(arg, 1);
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<unsigned char>
{
    static std::string invoke(const unsigned char& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, 1);
        }
        if (specifier == "b") {
            return toBin(arg, 1);
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<short>
{
    static std::string invoke(const short& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(short));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(short));
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<unsigned short>
{
    static std::string invoke(const unsigned short& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(unsigned short));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(unsigned short));
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<int>
{
    static std::string invoke(const int& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(int));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(int));
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<unsigned int>
{
    static std::string invoke(const unsigned int& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(unsigned int));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(unsigned int));
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<long>
{
    static std::string invoke(const long& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(long));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(long));
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<unsigned long>
{
    static std::string invoke(const unsigned long& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(unsigned long));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(unsigned long));
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<long long>
{
    static std::string invoke(const long long& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(long long));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(long long));
        }
        UNKNOWN_SPECIFIER();
    }
};

template<>
struct DefaultToString<unsigned long long>
{
    static std::string invoke(const unsigned long long& arg, std::string_view specifier)
    {
        if (specifier.empty() || specifier == "d") {
            return std::to_string(arg);
        }
        if (specifier == "x") {
            return toHex(arg, sizeof(unsigned long long));
        }
        if (specifier == "b") {
            return toBin(arg, sizeof(unsigned long long));
        }
        UNKNOWN_SPECIFIER();
    }
};

#undef UNKNOWN_SPECIFIER

template<>
struct DefaultToString<float>
{
    static std::string invoke(const float& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::to_string(arg);
    }
};

template<>
struct DefaultToString<double>
{
    static std::string invoke(const double& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::to_string(arg);
    }
};

template<>
struct DefaultToString<long double>
{
    static std::string invoke(const long double& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::to_string(arg);
    }
};

template<>
struct DefaultToString<std::string>
{
    static std::string invoke(const std::string& arg, [[maybe_unused]] std::string_view specifier)
    {
        return arg;
    }
};

template<>
struct DefaultToString<std::string_view>
{
    static std::string invoke(const std::string_view& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::string{arg};
    }
};

template<>
struct DefaultToString<const char*>
{
    static std::string invoke(const std::string_view& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::string{arg};
    }
};

template<>
struct DefaultToString<char*>
{
    static std::string invoke(const std::string_view& arg, [[maybe_unused]] std::string_view specifier)
    {
        return std::string{arg};
    }
};
} // namespace detail

template<typename T>
using DefaultToString = detail::DefaultToString<T>;

template<typename T>
struct ToString
{
    static std::string invoke(const T& arg, std::string_view specifier)
    {
        return detail::DefaultToString<T>::invoke(arg, specifier);
    }
};

template<typename ...ARGS>
std::string format(std::string_view format_string, ARGS&& ... args)
{
    using FA = detail::FormatArg;
    using f_t = FA::f_t;
    ASSERT(detail::getArgumentNum(format_string) == sizeof...(args), "argument number mismatch");
    if constexpr (sizeof...(args) == 0) {
        return std::string{format_string};
    } else {
        detail::FormatArgs<sizeof...(ARGS)> arg_array{
                {reinterpret_cast<const void*>(const_cast<FA::arg_t <ARGS>*>(&args)),
                 reinterpret_cast<f_t>(&ToString<std::remove_cv_t<std::remove_reference_t<ARGS>>>::invoke)}...};
        return detail::format(format_string, arg_array.data());
    }
}

} // namespace cami::lib

#define CAMI_DECLARE_FORMATTER(clazz)                                           \
    template<>                                                                  \
    struct cami::lib::ToString<clazz>                                           \
    {                                                                           \
        static std::string invoke(const clazz& clz, std::string_view specifier);\
    }

#endif //CAMI_LIB_FORMAT_H
