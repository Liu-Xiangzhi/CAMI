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

#ifndef CAMI_LIB_SLICE_H
#define CAMI_LIB_SLICE_H

#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>

namespace cami::lib {

template<typename T>
class Slice
{
    T* ptr;
    size_t size_;
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

    Slice() : ptr(nullptr), size_(0) {}

    Slice(T* ptr, size_t size) : ptr(ptr), size_(size) {}

    template<typename Container, typename = std::enable_if_t<std::is_same_v<typename Container::value_type, T>>>
    Slice(Container& container): ptr(container.data()), size_(container.size()) {} // NOLINT

    template<typename Container, typename = std::enable_if_t<std::is_same_v<typename Container::value_type const, T>>>
    Slice(const Container& container): ptr(container.data()), size_(container.size()) {} // NOLINT
public:

    [[nodiscard]] size_t length() const noexcept
    {
        return this->size_;
    }

    [[nodiscard]] size_t size() const noexcept
    {
        return this->size_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return this->size_ == 0;
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
        ASSERT(idx < this->size_,
               "index out of boundary. idx: "s + std::to_string(idx) + ", len: "s + std::to_string(this->size_));
        return this->ptr[idx];
    }

    T& operator[](size_t idx) noexcept
    {
        using namespace std::string_literals;
        ASSERT(idx < this->size_,
               "index out of boundary. idx: "s + std::to_string(idx) + ", len: "s + std::to_string(this->size_));
        return this->ptr[idx];
    }

    const T& back() const noexcept
    {
        return this->operator[](this->size_ - 1);
    }

    T& back() noexcept
    {
        return this->operator[](this->size_ - 1);
    }

    iterator begin() noexcept
    {
        return iterator{this->ptr};
    }

    iterator end() noexcept
    {
        return iterator{this->ptr + this->size_};
    }

    const_iterator begin() const noexcept
    {
        return const_iterator{this->ptr};
    }

    const_iterator end() const noexcept
    {
        return const_iterator{this->ptr + this->size_};
    }
};

template<typename T>
inline typename Slice<T>::iterator
operator+(typename Slice<T>::iterator::difference_type n, typename Slice<T>::iterator itr)
{
    return itr + n;
}

template<typename T>
inline typename Slice<T>::const_iterator
operator+(typename Slice<T>::iterator::difference_type n, typename Slice<T>::const_iterator itr)
{
    return itr + n;
}
} // namespace cami::lib
#endif //CAMI_LIB_SLICE_H
