// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

#include "mcl/assert.hpp"

namespace mcl {

template<typename T>
class intrusive_list;
template<typename T>
class intrusive_list_iterator;

template<typename T>
class intrusive_list_node {
public:
    bool is_sentinel() const
    {
        return is_sentinel_;
    }

protected:
    intrusive_list_node* next = nullptr;
    intrusive_list_node* prev = nullptr;
    bool is_sentinel_ = false;

    friend class intrusive_list<T>;
    friend class intrusive_list_iterator<T>;
    friend class intrusive_list_iterator<const T>;
};

template<typename T>
class intrusive_list_sentinel final : public intrusive_list_node<T> {
    using intrusive_list_node<T>::next;
    using intrusive_list_node<T>::prev;
    using intrusive_list_node<T>::is_sentinel_;

public:
    intrusive_list_sentinel()
    {
        next = this;
        prev = this;
        is_sentinel_ = true;
    }
};

template<typename T>
class intrusive_list_iterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    // If value_type is const, we want "const intrusive_list_node<value_type>", not "intrusive_list_node<const value_type>"
    using node_type = std::conditional_t<std::is_const<value_type>::value,
                                         const intrusive_list_node<std::remove_const_t<value_type>>,
                                         intrusive_list_node<value_type>>;
    using node_pointer = node_type*;
    using node_reference = node_type&;

    intrusive_list_iterator() = default;
    intrusive_list_iterator(const intrusive_list_iterator& other) = default;
    intrusive_list_iterator& operator=(const intrusive_list_iterator& other) = default;

    explicit intrusive_list_iterator(node_pointer list_node)
        : node(list_node) {}
    explicit intrusive_list_iterator(pointer data)
        : node(data) {}
    explicit intrusive_list_iterator(reference data)
        : node(&data) {}

    intrusive_list_iterator& operator++()
    {
        node = node->next;
        return *this;
    }
    intrusive_list_iterator& operator--()
    {
        node = node->prev;
        return *this;
    }
    intrusive_list_iterator operator++(int)
    {
        intrusive_list_iterator it(*this);
        ++*this;
        return it;
    }
    intrusive_list_iterator operator--(int)
    {
        intrusive_list_iterator it(*this);
        --*this;
        return it;
    }

    bool operator==(const intrusive_list_iterator& other) const
    {
        return node == other.node;
    }
    bool operator!=(const intrusive_list_iterator& other) const
    {
        return !operator==(other);
    }

    reference operator*() const
    {
        DEBUG_ASSERT(!node->is_sentinel());
        return static_cast<reference>(*node);
    }
    pointer operator->() const
    {
        return std::addressof(operator*());
    }

    node_pointer AsNodePointer() const
    {
        return node;
    }

private:
    friend class intrusive_list<T>;
    node_pointer node = nullptr;
};

template<typename T>
class intrusive_list {
public:
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;
    using value_type = T;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = intrusive_list_iterator<value_type>;
    using const_iterator = intrusive_list_iterator<const value_type>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * Inserts a node at the given location indicated by an iterator.
     *
     * @param location The location to insert the node.
     * @param new_node The node to add.
     */
    iterator insert(iterator location, pointer new_node)
    {
        return insert_before(location, new_node);
    }

    /**
     * Inserts a node at the given location, moving the previous
     * node occupant ahead of the one inserted.
     *
     * @param location The location to insert the new node.
     * @param new_node The node to insert into the list.
     */
    iterator insert_before(iterator location, pointer new_node)
    {
        auto existing_node = location.AsNodePointer();

        new_node->next = existing_node;
        new_node->prev = existing_node->prev;
        existing_node->prev->next = new_node;
        existing_node->prev = new_node;

        return iterator(new_node);
    }

    /**
     * Inserts a new node into the list ahead of the position indicated.
     *
     * @param position Location to insert the node in front of.
     * @param new_node The node to be inserted into the list.
     */
    iterator insert_after(iterator position, pointer new_node)
    {
        if (empty())
            return insert(begin(), new_node);

        return insert(++position, new_node);
    }

    /**
     * Add an entry to the start of the list.
     * @param node Node to add to the list.
     */
    void push_front(pointer node)
    {
        insert(begin(), node);
    }

    /**
     * Add an entry to the end of the list
     * @param node Node to add to the list.
     */
    void push_back(pointer node)
    {
        insert(end(), node);
    }

    /**
     * Erases the node at the front of the list.
     * @note Must not be called on an empty list.
     */
    void pop_front()
    {
        DEBUG_ASSERT(!empty());
        erase(begin());
    }

    /**
     * Erases the node at the back of the list.
     * @note Must not be called on an empty list.
     */
    void pop_back()
    {
        DEBUG_ASSERT(!empty());
        erase(--end());
    }

    /**
     * Removes a node from this list
     * @param it An iterator that points to the node to remove from list.
     */
    pointer remove(iterator& it)
    {
        DEBUG_ASSERT(it != end());

        pointer node = &*it++;

        node->prev->next = node->next;
        node->next->prev = node->prev;
#if !defined(NDEBUG)
        node->next = nullptr;
        node->prev = nullptr;
#endif

        return node;
    }

    /**
     * Removes a node from this list
     * @param it A constant iterator that points to the node to remove from list.
     */
    pointer remove(const iterator& it)
    {
        iterator copy = it;
        return remove(copy);
    }

    /**
     * Removes a node from this list.
     * @param node A pointer to the node to remove.
     */
    pointer remove(pointer node)
    {
        return remove(iterator(node));
    }

    /**
     * Removes a node from this list.
     * @param node A reference to the node to remove.
     */
    pointer remove(reference node)
    {
        return remove(iterator(node));
    }

    /**
     * Is this list empty?
     * @returns true if there are no nodes in this list.
     */
    bool empty() const
    {
        return root->next == root.get();
    }

    /**
     * Gets the total number of elements within this list.
     * @return the number of elements in this list.
     */
    size_type size() const
    {
        return static_cast<size_type>(std::distance(begin(), end()));
    }

    /**
     * Retrieves a reference to the node at the front of the list.
     * @note Must not be called on an empty list.
     */
    reference front()
    {
        DEBUG_ASSERT(!empty());
        return *begin();
    }

    /**
     * Retrieves a constant reference to the node at the front of the list.
     * @note Must not be called on an empty list.
     */
    const_reference front() const
    {
        DEBUG_ASSERT(!empty());
        return *begin();
    }

    /**
     * Retrieves a reference to the node at the back of the list.
     * @note Must not be called on an empty list.
     */
    reference back()
    {
        DEBUG_ASSERT(!empty());
        return *--end();
    }

    /**
     * Retrieves a constant reference to the node at the back of the list.
     * @note Must not be called on an empty list.
     */
    const_reference back() const
    {
        DEBUG_ASSERT(!empty());
        return *--end();
    }

    // Iterator interface
    iterator begin() { return iterator(root->next); }
    const_iterator begin() const { return const_iterator(root->next); }
    const_iterator cbegin() const { return begin(); }

    iterator end() { return iterator(root.get()); }
    const_iterator end() const { return const_iterator(root.get()); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const { return rbegin(); }

    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const { return rend(); }

    /**
     * Erases a node from the list, indicated by an iterator.
     * @param it The iterator that points to the node to erase.
     */
    iterator erase(iterator it)
    {
        remove(it);
        return it;
    }

    /**
     * Erases a node from this list.
     * @param node A pointer to the node to erase from this list.
     */
    iterator erase(pointer node)
    {
        return erase(iterator(node));
    }

    /**
     * Erases a node from this list.
     * @param node A reference to the node to erase from this list.
     */
    iterator erase(reference node)
    {
        return erase(iterator(node));
    }

    /**
     * Exchanges contents of this list with another list instance.
     * @param other The other list to swap with.
     */
    void swap(intrusive_list& other) noexcept
    {
        root.swap(other.root);
    }

private:
    std::shared_ptr<intrusive_list_node<T>> root = std::make_shared<intrusive_list_sentinel<T>>();
};

/**
 * Exchanges contents of an intrusive list with another intrusive list.
 * @tparam T The type of data being kept track of by the lists.
 * @param lhs The first list.
 * @param rhs The second list.
 */
template<typename T>
void swap(intrusive_list<T>& lhs, intrusive_list<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

}  // namespace mcl
