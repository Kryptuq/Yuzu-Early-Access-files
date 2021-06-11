/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

#include "dynarmic/common/assert.h"

namespace Dynarmic::Common {

template<typename T>
class IntrusiveList;
template<typename T>
class IntrusiveListIterator;

template<typename T>
class IntrusiveListNode {
public:
    bool IsSentinel() const {
        return is_sentinel;
    }

protected:
    IntrusiveListNode* next = nullptr;
    IntrusiveListNode* prev = nullptr;
    bool is_sentinel = false;

    friend class IntrusiveList<T>;
    friend class IntrusiveListIterator<T>;
    friend class IntrusiveListIterator<const T>;
};

template<typename T>
class IntrusiveListSentinel final : public IntrusiveListNode<T> {
    using IntrusiveListNode<T>::next;
    using IntrusiveListNode<T>::prev;
    using IntrusiveListNode<T>::is_sentinel;

public:
    IntrusiveListSentinel() {
        next = this;
        prev = this;
        is_sentinel = true;
    }
};

template<typename T>
class IntrusiveListIterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    // If value_type is const, we want "const IntrusiveListNode<value_type>", not "const IntrusiveListNode<const value_type>"
    using node_type = std::conditional_t<std::is_const<value_type>::value,
                                         const IntrusiveListNode<std::remove_const_t<value_type>>,
                                         IntrusiveListNode<value_type>>;
    using node_pointer = node_type*;
    using node_reference = node_type&;

    IntrusiveListIterator() = default;
    IntrusiveListIterator(const IntrusiveListIterator& other) = default;
    IntrusiveListIterator& operator=(const IntrusiveListIterator& other) = default;

    explicit IntrusiveListIterator(node_pointer list_node)
            : node(list_node) {
    }
    explicit IntrusiveListIterator(pointer data)
            : node(data) {
    }
    explicit IntrusiveListIterator(reference data)
            : node(&data) {
    }

    IntrusiveListIterator& operator++() {
        node = node->next;
        return *this;
    }
    IntrusiveListIterator& operator--() {
        node = node->prev;
        return *this;
    }
    IntrusiveListIterator operator++(int) {
        IntrusiveListIterator it(*this);
        ++*this;
        return it;
    }
    IntrusiveListIterator operator--(int) {
        IntrusiveListIterator it(*this);
        --*this;
        return it;
    }

    bool operator==(const IntrusiveListIterator& other) const {
        return node == other.node;
    }
    bool operator!=(const IntrusiveListIterator& other) const {
        return !operator==(other);
    }

    reference operator*() const {
        DEBUG_ASSERT(!node->IsSentinel());
        return static_cast<reference>(*node);
    }
    pointer operator->() const {
        return std::addressof(operator*());
    }

    node_pointer AsNodePointer() const {
        return node;
    }

private:
    friend class IntrusiveList<T>;
    node_pointer node = nullptr;
};

template<typename T>
class IntrusiveList {
public:
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;
    using value_type = T;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = IntrusiveListIterator<value_type>;
    using const_iterator = IntrusiveListIterator<const value_type>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * Inserts a node at the given location indicated by an iterator.
     *
     * @param location The location to insert the node.
     * @param new_node The node to add.
     */
    iterator insert(iterator location, pointer new_node) {
        return insert_before(location, new_node);
    }

    /**
     * Inserts a node at the given location, moving the previous
     * node occupant ahead of the one inserted.
     *
     * @param location The location to insert the new node.
     * @param new_node The node to insert into the list.
     */
    iterator insert_before(iterator location, pointer new_node) {
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
    iterator insert_after(iterator position, pointer new_node) {
        if (empty())
            return insert(begin(), new_node);

        return insert(++position, new_node);
    }

    /**
     * Add an entry to the start of the list.
     * @param node Node to add to the list.
     */
    void push_front(pointer node) {
        insert(begin(), node);
    }

    /**
     * Add an entry to the end of the list
     * @param node Node to add to the list.
     */
    void push_back(pointer node) {
        insert(end(), node);
    }

    /**
     * Erases the node at the front of the list.
     * @note Must not be called on an empty list.
     */
    void pop_front() {
        DEBUG_ASSERT(!empty());
        erase(begin());
    }

    /**
     * Erases the node at the back of the list.
     * @note Must not be called on an empty list.
     */
    void pop_back() {
        DEBUG_ASSERT(!empty());
        erase(--end());
    }

    /**
     * Removes a node from this list
     * @param it An iterator that points to the node to remove from list.
     */
    pointer remove(iterator& it) {
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
    pointer remove(const iterator& it) {
        iterator copy = it;
        return remove(copy);
    }

    /**
     * Removes a node from this list.
     * @param node A pointer to the node to remove.
     */
    pointer remove(pointer node) {
        return remove(iterator(node));
    }

    /**
     * Removes a node from this list.
     * @param node A reference to the node to remove.
     */
    pointer remove(reference node) {
        return remove(iterator(node));
    }

    /**
     * Is this list empty?
     * @returns true if there are no nodes in this list.
     */
    bool empty() const {
        return root->next == root.get();
    }

    /**
     * Gets the total number of elements within this list.
     * @return the number of elements in this list.
     */
    size_type size() const {
        return static_cast<size_type>(std::distance(begin(), end()));
    }

    /**
     * Retrieves a reference to the node at the front of the list.
     * @note Must not be called on an empty list.
     */
    reference front() {
        DEBUG_ASSERT(!empty());
        return *begin();
    }

    /**
     * Retrieves a constant reference to the node at the front of the list.
     * @note Must not be called on an empty list.
     */
    const_reference front() const {
        DEBUG_ASSERT(!empty());
        return *begin();
    }

    /**
     * Retrieves a reference to the node at the back of the list.
     * @note Must not be called on an empty list.
     */
    reference back() {
        DEBUG_ASSERT(!empty());
        return *--end();
    }

    /**
     * Retrieves a constant reference to the node at the back of the list.
     * @note Must not be called on an empty list.
     */
    const_reference back() const {
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
    iterator erase(iterator it) {
        remove(it);
        return it;
    }

    /**
     * Erases a node from this list.
     * @param node A pointer to the node to erase from this list.
     */
    iterator erase(pointer node) {
        return erase(iterator(node));
    }

    /**
     * Erases a node from this list.
     * @param node A reference to the node to erase from this list.
     */
    iterator erase(reference node) {
        return erase(iterator(node));
    }

    /**
     * Exchanges contents of this list with another list instance.
     * @param other The other list to swap with.
     */
    void swap(IntrusiveList& other) noexcept {
        root.swap(other.root);
    }

private:
    std::shared_ptr<IntrusiveListNode<T>> root = std::make_shared<IntrusiveListSentinel<T>>();
};

/**
 * Exchanges contents of an intrusive list with another intrusive list.
 * @tparam T The type of data being kept track of by the lists.
 * @param lhs The first list.
 * @param rhs The second list.
 */
template<typename T>
void swap(IntrusiveList<T>& lhs, IntrusiveList<T>& rhs) noexcept {
    lhs.swap(rhs);
}

}  // namespace Dynarmic::Common
