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

#ifndef CAMI_LIB_ARRAY_H
#define CAMI_LIB_ARRAY_H

#include <cstddef>
#include <lib/assert.h>
#include <string>
#include <iterator>
#include <vector>
#include <set>
#include <cstring>

namespace cami::lib {

template<typename T>
class Array
{
    T* ptr;
    size_t size;
public:
    template<typename P>
    class Iterator
    {
    public:
        using difference_type = size_t;
        using value_type = P;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::random_access_iterator_tag;
    private:
        P* ptr;
    public:
        explicit Iterator(P* ptr) : ptr(ptr) {}

        Iterator& operator+(difference_type n) const noexcept
        {
            ASSERT(this->ptr != nullptr || n == 0, "operate empty array");
            return Iterator{this->ptr + n};
        }

        Iterator& operator-(difference_type n) const noexcept
        {
            ASSERT(this->ptr != nullptr || n == 0, "operate empty array");
            return Iterator{this->ptr - n};
        }

        difference_type operator-(Iterator that) const noexcept
        {
            if (this->ptr == nullptr) {
                ASSERT(that.ptr == nullptr, "operate empty array");
                return 0;
            }
            ASSERT(that.ptr != nullptr, "operate empty array");
            return this->ptr - that.ptr;
        }

        Iterator& operator+=(difference_type n) noexcept
        {
            ASSERT(this->ptr != nullptr || n == 0, "operate empty array");
            this->ptr += n;
            return *this;
        }

        Iterator& operator-=(difference_type n) noexcept
        {
            ASSERT(this->ptr != nullptr || n == 0, "operate empty array");
            this->ptr -= n;
            return *this;
        }

        Iterator& operator++()
        {
            ASSERT(this->ptr != nullptr, "operate empty array");
            this->ptr++;
            return *this;
        }

        Iterator& operator--()
        {
            ASSERT(this->ptr != nullptr, "operate empty array");
            this->ptr--;
            return *this;
        }

        Iterator operator++(int) // NOLINT
        {
            ASSERT(this->ptr != nullptr, "operate empty array");
            auto p = this->ptr;
            this->ptr++;
            return Iterator{p};
        }

        Iterator operator--(int) // NOLINT
        {
            ASSERT(this->ptr != nullptr, "operate empty array");
            auto p = this->ptr;
            this->ptr--;
            return Iterator{p};
        }

        pointer operator->() const noexcept
        {
            return this->ptr;
        }

        reference operator*() const noexcept
        {
            ASSERT(this->ptr != nullptr, "operate empty array");
            return *this->ptr;
        }

        reference operator[](difference_type n) const noexcept
        {
            return this->operator*(this->operator+(n));
        }

        bool operator==(Iterator that) const noexcept
        {
            return this->ptr == that.ptr;
        }

        bool operator!=(Iterator that) const noexcept
        {
            return this->ptr != that.ptr;
        }
    };

public:
    using iterator = Iterator<T>;
    using const_iterator = Iterator<const T>;

    Array() : ptr(nullptr), size(0) {}

    explicit Array(size_t size) : ptr(alloc(size)), size(size) {}

    Array(std::initializer_list<T> list) : ptr(alloc(list.size())), size(list.size())
    {
        auto itr = list.begin();
        for (size_t i = 0; i < this->size; ++i, ++itr) {
            new(&this->ptr[i]) T{*itr};
        }
    }

    Array(const Array& that) : ptr(alloc(that.size)), size(that.size)
    {
        for (size_t i = 0; i < this->size; ++i) {
            new(&this->ptr[i]) T{that.ptr[i]};
        }
    }

    Array(Array&& that) noexcept : ptr(that.ptr), size(that.size)
    {
        that.ptr = nullptr;
        that.size = 0;
    }

    Array& operator=(const Array&) = delete;
    Array& operator=(Array&&) noexcept = delete;

    ~Array()
    {
        if (this->ptr == nullptr) {
            return;
        }
        for (size_t i = 0; i < this->size; ++i) {
            this->ptr[i].~T();
        }
        free(this->ptr);
        this->ptr = nullptr;
    }

    static T* alloc(size_t size)
    {
        return size > 0 ? reinterpret_cast<T*>(malloc(sizeof(T) * size)) : nullptr;
    }

public:
    void assign(const Array<T>& that)
    {
        if (this == &that) [[unlikely]] {
            return;
        }
        this->~Array();
        new(this) Array{that};
    }

    void assign(Array<T>&& that)
    {
        if (this == &that) [[unlikely]] {
            return;
        }
        this->~Array();
        new(this) Array{std::move(that)};
    }

    void clear()
    {
        this->~Array();
        new(this) Array(0);
    }

    template<typename ...ARGS>
    void init(size_t idx, ARGS&& ... args)
    {
        new(&this->operator[](idx)) T{std::forward<ARGS>(args)...};
    }

    [[nodiscard]] size_t length() const noexcept
    {
        return this->size;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return this->size == 0;
    }

    const T* data() const noexcept
    {
        return this->ptr;
    }

    T* data() noexcept
    {
        return this->ptr;
    }

    const T& operator[](size_t idx) const noexcept
    {
        using namespace std::string_literals;
        ASSERT(idx < this->size,
               "index out of boundary. idx: "s + std::to_string(idx) + ", len: "s + std::to_string(this->size));
        return this->ptr[idx];
    }

    T& operator[](size_t idx) noexcept
    {
        using namespace std::string_literals;
        ASSERT(idx < this->size,
               "index out of boundary. idx: "s + std::to_string(idx) + ", len: "s + std::to_string(this->size));
        return this->ptr[idx];
    }

    const T& back() const noexcept
    {
        return this->operator[](this->size - 1);
    }

    T& back() noexcept
    {
        return this->operator[](this->size - 1);
    }

    iterator begin() noexcept
    {
        return iterator{this->ptr};
    }

    iterator end() noexcept
    {
        return iterator{this->size > 0 ? this->ptr + this->size : nullptr};
    }

    const_iterator begin() const noexcept
    {
        return const_iterator{this->ptr};
    }

    const_iterator end() const noexcept
    {
        return const_iterator{this->size > 0 ? this->ptr + this->size : nullptr};
    }

    static Array fromVector(const std::vector<T>& vec)
    {
        Array<T> result(vec.size());
        // just trust your compiler could be able to optimize this for loop into `memcpy`
        //   or SIMD instructions for trivial_copy_constructable instance :)
        for (size_t i = 0; i < vec.size(); ++i) {
            result.init(i, vec[i]);
        }
        return result;
    }

    static Array fromVector(std::vector<T>&& vec)
    {
        Array<T> result(vec.size());
        for (size_t i = 0; i < vec.size(); ++i) {
            result.init(i, std::move(vec[i]));
        }
        return result;
    }

    static Array fromSet(const std::set<T>& set)
    {
        Array<T> result(set.size());
        size_t cnt = 0;
        for (const auto& item: set) {
            result.init(cnt++, item);
        }
        return result;
    }
};

template<typename T>
inline typename Array<T>::iterator
operator+(typename Array<T>::iterator::difference_type n, typename Array<T>::iterator itr)
{
    return itr + n;
}

template<typename T>
inline typename Array<T>::const_iterator
operator+(typename Array<T>::iterator::difference_type n, typename Array<T>::const_iterator itr)
{
    return itr + n;
}

} // namespace cami::lib

#endif //CAMI_LIB_ARRAY_H
