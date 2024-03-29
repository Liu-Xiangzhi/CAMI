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

#ifndef CAMI_LIB_LIST_H
#define CAMI_LIB_LIST_H

#include <lib/assert.h>
#include <utility>
#include <type_traits>
#include <iterator>

namespace cami::lib {

namespace detail {
struct NodeHeader
{
    NodeHeader* next = nullptr;
    NodeHeader() = default;
    NodeHeader(const NodeHeader&) = delete;
    NodeHeader& operator=(const NodeHeader&) = delete;
    NodeHeader& operator=(NodeHeader&& that) noexcept = delete;

    NodeHeader(NodeHeader&& that) noexcept : next(that.next)
    {
        that.next = nullptr;
    }

    void insertHead(NodeHeader* node)
    {
        node->next = this->next;
        this->next = node;
    }

    void eraseHead()
    {
        ASSERT(!this->empty(), "erase empty list");
        this->next = this->next->next;
    }

    static void eraseAfter(NodeHeader* header)
    {
        ASSERT(header != nullptr && header->next != nullptr, "erase non-exist node");
        header->next = header->next->next;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return this->next == nullptr;
    }
};

template<typename T>
struct Node : public NodeHeader
{
    using element_t = T;

    T element;

    template<typename ...ARGS>
    explicit Node(std::in_place_t, ARGS&& ...args) : element(std::forward<ARGS>(args)...) {}

    explicit Node(const T& element) : element(element) {}
};

template<typename T>
struct ElementType
{
    using type = typename T::element_t;
};
template<typename T>
struct ElementType<const T>
{
    using type = typename T::element_t const;
};

template<typename T>
using element_t = typename ElementType<T>::type;

} // namespace detail


/// List follow the naming convention of C++ standard library

template<typename T>
class List
{
    template<typename T_header, typename T_node>
    class Iterator
    {
    public:
        using difference_type = size_t;
        using value_type = typename detail::element_t<T_node>;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::input_iterator_tag;
    private:
        T_header* node;

        friend class List<T>;

    public:
        static_assert(std::is_base_of<T_header, T_node>::value, "T_node must be a derived type of T_header");

        explicit Iterator(T_header* node) : node(node) {}

        pointer operator->() const
        {
            return &static_cast<T_node*>(this->node)->element;
        }

        reference operator*() const
        {
            return static_cast<T_node*>(this->node)->element;
        }

        Iterator& operator++()
        {
            this->node = this->node->next;
            return *this;
        }

        Iterator operator++(int) // NOLINT
        {
            auto n = this->node;
            this->node = this->node->next;
            return Iterator{n};
        }

        bool operator==(Iterator that) const noexcept
        {
            return this->node == that.node;
        }

        bool operator!=(Iterator that) const noexcept
        {
            return this->node != that.node;
        }
    };

public:
    using iterator = Iterator<detail::NodeHeader, detail::Node<T>>;
    using const_iterator = Iterator<const detail::NodeHeader, const detail::Node<T>>;
protected:
    detail::NodeHeader header;
public:
    List() = default;
    List(List&& that) noexcept = default;

    ~List()
    {
        this->clear();
    }

    [[nodiscard]] iterator begin() noexcept // NOLINT
    {
        return iterator{this->header.next};
    }

    iterator end() noexcept
    {
        return iterator{nullptr};
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return const_iterator{this->header.next};
    }

    [[nodiscard]] const_iterator end() const noexcept
    {
        return const_iterator{nullptr};
    }

    const T& head() const
    {
        ASSERT(!this->empty(), "read empty list");
        return *this->begin();
    }

    T& head()
    {
        ASSERT(!this->empty(), "read empty list");
        return *this->begin();
    }

    void insert_head(T&& element)
    {
        this->header.insertHead(new detail::Node<T>{std::move(element)});
    }

    void push_front(T&& element)
    {
        this->insert_head(std::move(element));
    }

    void insert_head(const T& element)
    {
        this->header.insertHead(new detail::Node<T>{element});
    }

    void push_front(const T& element)
    {
        this->insert_head(element);
    }

    template<typename ...ARGS>
    void emplace_front(ARGS&& ...args)
    {
        this->header.insertHead(new detail::Node<T>{std::in_place, std::forward<ARGS>(args)...});
    }

    void erase_head()
    {
        ASSERT(!this->empty(), "erase empty list");
        auto node = static_cast<detail::Node<T>*>(this->header.next);
        this->header.eraseHead();
        delete node;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return this->header.empty();
    }

    void clear()
    {
        while (!this->empty()) {
            this->erase_head();
        }
    }

    template<typename T_pred>
    void erase_if(const T_pred& pred)
    {
        auto* n = &this->header;
        while (n->next != nullptr) {
            if (pred(static_cast<detail::Node<T>*>(n->next)->element)) {
                auto node = static_cast<detail::Node<T>*>(n->next);
                detail::NodeHeader::eraseAfter(n);
                delete node;
                if (n->next == nullptr) {
                    break;
                }
            }
            n = n->next;
        }
    }
};

} // namespace cami::lib

#endif //CAMI_LIB_LIST_H
