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

#ifndef CAMI_LIB_SHARED_PTR_H
#define CAMI_LIB_SHARED_PTR_H

#include <cstddef>
#include <lib/assert.h>

namespace cami::lib {
/// `SharedPtr` is(intentionally) not thread-safe
template<typename T>
class SharedPtr
{
    T* value;
    size_t* ref_cnt;
public:
    SharedPtr() : value(nullptr), ref_cnt(nullptr) {}

    SharedPtr(T* value, size_t* ref_cnt) : value(value), ref_cnt(ref_cnt) {}

    ~SharedPtr()
    {
        if (this->ref_cnt == nullptr) {
            return;
        }
        if (--*this->ref_cnt == 0) {
            delete this->value;
            delete this->ref_cnt;
            this->ref_cnt = nullptr;
        }
    }

    SharedPtr(SharedPtr&& that) noexcept : value(that.value), ref_cnt(that.ref_cnt)
    {
        that.ref_cnt = nullptr;
    }

    SharedPtr(const SharedPtr& that) : value(that.value), ref_cnt(that.ref_cnt)
    {
        ++*this->ref_cnt;
    }

    SharedPtr& operator=(SharedPtr&& that) noexcept
    {
        if (this != &that) [[likely]] {
            this->~SharedPtr();
            this->value = that.value;
            this->ref_cnt = that.ref_cnt;
            that.ref_cnt = nullptr;
        }
        return *this;
    }

    SharedPtr& operator=(const SharedPtr& that)
    {
        if (this != &that) [[likely]] {
            this->~SharedPtr();
            this->value = that.value;
            this->ref_cnt = that.ref_cnt;
            ++*this->ref_cnt;
        }
        return *this;
    }

    T& operator*() const noexcept
    {
        ASSERT(this->value != nullptr, "access empty shared pointer");
        return *this->value;
    }

    T* operator->() const noexcept
    {
        ASSERT(this->value != nullptr, "access empty shared pointer");
        return this->value;
    }
};

template<typename T, typename ...ARGS>
SharedPtr<T> makeShared(ARGS&& ... args)
{
    return SharedPtr<T>{new T{std::forward<ARGS>(args)...}, new size_t{1}};
}

} // namespace cami::lib

#endif //CAMI_LIB_SHARED_PTR_H
