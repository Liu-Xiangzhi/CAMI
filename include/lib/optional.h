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

#ifndef CAMI_LIB_OPTIONAL_H
#define CAMI_LIB_OPTIONAL_H

#include <optional>
#include <memory>
#include <lib/assert.h>

namespace cami::lib {
namespace detail {
template<typename T>
struct DefaultHelper;

template<typename T>
struct DefaultHelper<T*>
{
    static bool isNull(const T* value) noexcept
    {
        return value == nullptr;
    }

    static T* makeNull()
    {
        return nullptr;
    }
};

template<typename T>
struct DefaultHelper<std::unique_ptr<T>>
{
    static std::unique_ptr<T> makeNull() noexcept
    {
        return {};
    }

    static bool isNull(const std::unique_ptr<T>& val) noexcept
    {
        return val.get() == nullptr;
    }
};

template<typename T>
struct DefaultHelper<std::shared_ptr<T>>
{
    static std::shared_ptr<T> makeNull() noexcept
    {
        return {};
    }

    static bool isNull(const std::shared_ptr<T>& val) noexcept
    {
        return val.get() == nullptr;
    }
};

template<typename T>
struct DefaultHelper<std::weak_ptr<T>>
{
    static std::weak_ptr<T> makeNull() noexcept
    {
        return {};
    }

    static bool isNull(const std::weak_ptr<T>& val) noexcept
    {
        return val.get() == nullptr;
    }
};

template<typename T, typename T_helper>
struct HelperTrait
{
    using type = T_helper;
};
template<typename T>
struct HelperTrait<T, void>
{
    using type = DefaultHelper<T>;
};

} // namespace detail


/// Optional is just a wrapper for self-nullable types
/// e.g. there's a C style function f which return type is int
///  and returns negative value if error otherwise positive value,
///  then it can be written as(in C++ style) `Optional<int> f();`.
///  But if f may return any value in range of int while error didn't happen,
///  it should be written as `std::optional<int> f();` instead.
/// Optional follows the naming convention of C++ standard library
template<typename T, typename T_helper = void>
class Optional
{
    T val;
    using helper_t = typename detail::HelperTrait<T, T_helper>::type;
public:
    constexpr Optional() noexcept : val(helper_t::makeNull()) {}

    constexpr Optional(std::nullopt_t) noexcept : Optional() {} // NOLINT

    constexpr Optional(const T& value) : val(value) {} // NOLINT

    constexpr Optional(T&& value) noexcept : val(std::move(value)) {} // NOLINT

    template<typename ...ARGS>
    constexpr explicit Optional(std::in_place_t, ARGS... args) : val(std::forward<ARGS>(args)...) {}

    constexpr Optional(const Optional<T, T_helper>& that) = default;
    constexpr Optional(Optional<T, T_helper>&& that) noexcept = default;
    constexpr Optional& operator=(const Optional<T, T_helper>& that) = default;
    constexpr Optional& operator=(Optional<T, T_helper>&& that) noexcept = default;
    ~Optional() = default;

    constexpr explicit operator bool() const noexcept
    {
        return this->has_value();
    }

    [[nodiscard]] constexpr bool has_value() const noexcept
    {
        return !helper_t::isNull(this->val);
    }

    constexpr const T* operator->() const noexcept
    {
        ASSERT(this->has_value(), "unwrap empty optional");
        return &this->val;
    }

    constexpr const T& operator*() const noexcept
    {
        ASSERT(this->has_value(), "unwrap empty optional");
        return this->val;
    }

    constexpr T* operator->() noexcept
    {
        ASSERT(this->has_value(), "unwrap empty optional");
        return &this->val;
    }

    constexpr T& operator*() noexcept
    {
        ASSERT(this->has_value(), "unwrap empty optional");
        return this->val;
    }

    constexpr const T& value() const
    {
        if (helper_t::isNull(this->val)) {
            throw std::bad_optional_access{};
        }
        return this->val;
    }

    constexpr T& value()
    {
        if (helper_t::isNull(this->val)) {
            throw std::bad_optional_access{};
        }
        return this->val;
    }
};

} // namespace cami::lib

#endif //CAMI_LIB_OPTIONAL_H
