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

#ifndef CAMI_FOUNDATION_DOWNCAST_H
#define CAMI_FOUNDATION_DOWNCAST_H

#ifdef NDEBUG
#define DEBUG_VIRTUAL
#else
#define DEBUG_VIRTUAL virtual
#endif

#include <memory>
#include <type_traits>
namespace cami::detail {
template<typename T>
struct IsSmartPointer
{
    static constexpr bool value = false;
};
template<typename T>
struct IsSmartPointer<std::unique_ptr<T>>
{
    static constexpr bool value = true;
};
template<typename T>
struct IsSmartPointer<std::shared_ptr<T>>
{
    static constexpr bool value = true;
};
template<typename T>
struct IsSmartPointer<std::weak_ptr<T>>
{
    static constexpr bool value = true;
};
template<typename T>
constexpr bool is_smart_ptr_v = IsSmartPointer<T>::value;


template<typename T, typename P, typename = void>
struct DownCast
{
    static_assert(!std::is_same_v<T, T>, "cannot dynamic_cast non-reference and non-pointer expression");
};

template<typename T, typename P>
struct DownCast<T, P, std::enable_if_t<std::is_pointer_v<T> && std::is_pointer_v<std::remove_reference_t<P>>>>
{
    static T f(P expr)
    {
#ifdef NDEBUG
        return static_cast<T>(expr);
#else
        auto res = dynamic_cast<T>(expr);
        ASSERT(res != nullptr, "dynamic cast failed");
        return res;
#endif
    }
};

template<typename T, typename P>
struct DownCast<T, P, std::enable_if_t<
        std::is_reference_v<T> && std::is_reference_v<P> &&
        !is_smart_ptr_v<std::remove_cv_t<std::remove_reference_t<P>>>>>
{
    static T f(P expr)
    {
#ifdef NDEBUG
        return static_cast<T>(expr);
#else
        auto res = dynamic_cast<std::remove_reference_t<T>*>(&expr);
        ASSERT(res != nullptr, "dynamic cast failed");
        return *res;
#endif
    }
};

template<typename T, typename P>
struct DownCast<T, P, std::enable_if_t<
        std::is_pointer_v<T> && is_smart_ptr_v<std::remove_cv_t<std::remove_reference_t<P>>>>>
{
    static T f(P expr)
    {
#ifdef NDEBUG
        return static_cast<T>(expr.get());
#else
        auto res = dynamic_cast<T>(expr.get());
        ASSERT(res != nullptr, "dynamic cast failed");
        return res;
#endif
    }
};

template<typename T, typename P>
struct DownCast<T, P, std::enable_if_t<
        std::is_reference_v<T> && is_smart_ptr_v<std::remove_cv_t<std::remove_reference_t<P>>>>>
{
    static T f(P expr)
    {
#ifdef NDEBUG
        return static_cast<T>(*expr.get());
#else
        auto res = dynamic_cast<std::remove_reference_t<T>*>(expr.get());
        ASSERT(res != nullptr, "dynamic cast failed");
        return *res;
#endif
    }
};
} // namespace detail

namespace cami {
template<typename T, typename P>
T down_cast(P&& p)
{
    return ::cami::detail::DownCast<T, P&&>::f(p);
}

} // namespace cami

#endif //CAMI_FOUNDATION_DOWNCAST_H
