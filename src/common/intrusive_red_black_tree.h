// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/alignment.h"
#include "common/common_funcs.h"
#include "common/parent_of_member.h"
#include "common/tree.h"

namespace Common {

namespace impl {

class IntrusiveRedBlackTreeImpl;

}

#pragma pack(push, 4)
struct IntrusiveRedBlackTreeNode {
    YUZU_NON_COPYABLE(IntrusiveRedBlackTreeNode);

public:
    using RBEntry = freebsd::RBEntry<IntrusiveRedBlackTreeNode>;

private:
    RBEntry m_entry;

public:
    explicit IntrusiveRedBlackTreeNode() = default;

    [[nodiscard]] constexpr RBEntry& GetRBEntry() {
        return m_entry;
    }
    [[nodiscard]] constexpr const RBEntry& GetRBEntry() const {
        return m_entry;
    }

    constexpr void SetRBEntry(const RBEntry& entry) {
        m_entry = entry;
    }
};
static_assert(sizeof(IntrusiveRedBlackTreeNode) ==
              3 * sizeof(void*) + std::max<size_t>(sizeof(freebsd::RBColor), 4));
#pragma pack(pop)

template <class T, class Traits, class Comparator>
class IntrusiveRedBlackTree;

namespace impl {

class IntrusiveRedBlackTreeImpl {
    YUZU_NON_COPYABLE(IntrusiveRedBlackTreeImpl);

private:
    template <class, class, class>
    friend class ::Common::IntrusiveRedBlackTree;

private:
    using RootType = freebsd::RBHead<IntrusiveRedBlackTreeNode>;

private:
    RootType m_root;

public:
    template <bool Const>
    class Iterator;

    using value_type = IntrusiveRedBlackTreeNode;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    template <bool Const>
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename IntrusiveRedBlackTreeImpl::value_type;
        using difference_type = typename IntrusiveRedBlackTreeImpl::difference_type;
        using pointer = typename std::conditional<Const, IntrusiveRedBlackTreeImpl::const_pointer,
                                                  IntrusiveRedBlackTreeImpl::pointer>::type;
        using reference =
            typename std::conditional<Const, IntrusiveRedBlackTreeImpl::const_reference,
                                      IntrusiveRedBlackTreeImpl::reference>::type;

    private:
        pointer m_node;

    public:
        constexpr explicit Iterator(pointer n) : m_node(n) {}

        constexpr bool operator==(const Iterator& rhs) const {
            return m_node == rhs.m_node;
        }

        constexpr bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }

        constexpr pointer operator->() const {
            return m_node;
        }

        constexpr reference operator*() const {
            return *m_node;
        }

        constexpr Iterator& operator++() {
            m_node = GetNext(m_node);
            return *this;
        }

        constexpr Iterator& operator--() {
            m_node = GetPrev(m_node);
            return *this;
        }

        constexpr Iterator operator++(int) {
            const Iterator it{*this};
            ++(*this);
            return it;
        }

        constexpr Iterator operator--(int) {
            const Iterator it{*this};
            --(*this);
            return it;
        }

        constexpr operator Iterator<true>() const {
            return Iterator<true>(m_node);
        }
    };

private:
    constexpr bool EmptyImpl() const {
        return m_root.IsEmpty();
    }

    constexpr IntrusiveRedBlackTreeNode* GetMinImpl() const {
        return freebsd::RB_MIN(const_cast<RootType&>(m_root));
    }

    constexpr IntrusiveRedBlackTreeNode* GetMaxImpl() const {
        return freebsd::RB_MAX(const_cast<RootType&>(m_root));
    }

    constexpr IntrusiveRedBlackTreeNode* RemoveImpl(IntrusiveRedBlackTreeNode* node) {
        return freebsd::RB_REMOVE(m_root, node);
    }

public:
    static constexpr IntrusiveRedBlackTreeNode* GetNext(IntrusiveRedBlackTreeNode* node) {
        return freebsd::RB_NEXT(node);
    }

    static constexpr IntrusiveRedBlackTreeNode* GetPrev(IntrusiveRedBlackTreeNode* node) {
        return freebsd::RB_PREV(node);
    }

    static constexpr IntrusiveRedBlackTreeNode const* GetNext(
        IntrusiveRedBlackTreeNode const* node) {
        return static_cast<const IntrusiveRedBlackTreeNode*>(
            GetNext(const_cast<IntrusiveRedBlackTreeNode*>(node)));
    }

    static constexpr IntrusiveRedBlackTreeNode const* GetPrev(
        IntrusiveRedBlackTreeNode const* node) {
        return static_cast<const IntrusiveRedBlackTreeNode*>(
            GetPrev(const_cast<IntrusiveRedBlackTreeNode*>(node)));
    }

public:
    constexpr IntrusiveRedBlackTreeImpl() = default;

    // Iterator accessors.
    constexpr iterator begin() {
        return iterator(this->GetMinImpl());
    }

    constexpr const_iterator begin() const {
        return const_iterator(this->GetMinImpl());
    }

    constexpr iterator end() {
        return iterator(static_cast<IntrusiveRedBlackTreeNode*>(nullptr));
    }

    constexpr const_iterator end() const {
        return const_iterator(static_cast<const IntrusiveRedBlackTreeNode*>(nullptr));
    }

    constexpr const_iterator cbegin() const {
        return this->begin();
    }

    constexpr const_iterator cend() const {
        return this->end();
    }

    constexpr iterator iterator_to(reference ref) {
        return iterator(std::addressof(ref));
    }

    constexpr const_iterator iterator_to(const_reference ref) const {
        return const_iterator(std::addressof(ref));
    }

    // Content management.
    constexpr bool empty() const {
        return this->EmptyImpl();
    }

    constexpr reference back() {
        return *this->GetMaxImpl();
    }

    constexpr const_reference back() const {
        return *this->GetMaxImpl();
    }

    constexpr reference front() {
        return *this->GetMinImpl();
    }

    constexpr const_reference front() const {
        return *this->GetMinImpl();
    }

    constexpr iterator erase(iterator it) {
        auto cur = std::addressof(*it);
        auto next = GetNext(cur);
        this->RemoveImpl(cur);
        return iterator(next);
    }
};

} // namespace impl

template <typename T>
concept HasRedBlackKeyType = requires {
    { std::is_same<typename T::RedBlackKeyType, void>::value } -> std::convertible_to<bool>;
};

namespace impl {

    template <typename T, typename Default>
    consteval auto* GetRedBlackKeyType() {
        if constexpr (HasRedBlackKeyType<T>) {
            return static_cast<typename T::RedBlackKeyType*>(nullptr);
        } else {
            return static_cast<Default*>(nullptr);
        }
    }

} // namespace impl

template <typename T, typename Default>
using RedBlackKeyType =
    typename std::remove_pointer<decltype(impl::GetRedBlackKeyType<T, Default>())>::type;

template <class T, class Traits, class Comparator>
class IntrusiveRedBlackTree {
    YUZU_NON_COPYABLE(IntrusiveRedBlackTree);

public:
    using ImplType = impl::IntrusiveRedBlackTreeImpl;

private:
    ImplType m_impl;

public:
    template <bool Const>
    class Iterator;

    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    using key_type = RedBlackKeyType<Comparator, value_type>;
    using const_key_pointer = const key_type*;
    using const_key_reference = const key_type&;

    template <bool Const>
    class Iterator {
    public:
        friend class IntrusiveRedBlackTree<T, Traits, Comparator>;

        using ImplIterator =
            typename std::conditional<Const, ImplType::const_iterator, ImplType::iterator>::type;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename IntrusiveRedBlackTree::value_type;
        using difference_type = typename IntrusiveRedBlackTree::difference_type;
        using pointer = std::conditional_t<Const, IntrusiveRedBlackTree::const_pointer,
                                           IntrusiveRedBlackTree::pointer>;
        using reference = std::conditional_t<Const, IntrusiveRedBlackTree::const_reference,
                                             IntrusiveRedBlackTree::reference>;

    private:
        ImplIterator m_impl;

    private:
        constexpr explicit Iterator(ImplIterator it) : m_impl(it) {}

        constexpr explicit Iterator(typename ImplIterator::pointer p) : m_impl(p) {}

        constexpr ImplIterator GetImplIterator() const {
            return m_impl;
        }

    public:
        constexpr bool operator==(const Iterator& rhs) const {
            return m_impl == rhs.m_impl;
        }

        constexpr bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }

        constexpr pointer operator->() const {
            return Traits::GetParent(std::addressof(*m_impl));
        }

        constexpr reference operator*() const {
            return *Traits::GetParent(std::addressof(*m_impl));
        }

        constexpr Iterator& operator++() {
            ++m_impl;
            return *this;
        }

        constexpr Iterator& operator--() {
            --m_impl;
            return *this;
        }

        constexpr Iterator operator++(int) {
            const Iterator it{*this};
            ++m_impl;
            return it;
        }

        constexpr Iterator operator--(int) {
            const Iterator it{*this};
            --m_impl;
            return it;
        }

        constexpr operator Iterator<true>() const {
            return Iterator<true>(m_impl);
        }
    };

private:
    static constexpr int CompareImpl(const IntrusiveRedBlackTreeNode* lhs,
                                     const IntrusiveRedBlackTreeNode* rhs) {
        return Comparator::Compare(*Traits::GetParent(lhs), *Traits::GetParent(rhs));
    }

    static constexpr int CompareKeyImpl(const_key_reference key,
                                        const IntrusiveRedBlackTreeNode* rhs) {
        return Comparator::Compare(key, *Traits::GetParent(rhs));
    }

    // Define accessors using RB_* functions.
    constexpr IntrusiveRedBlackTreeNode* InsertImpl(IntrusiveRedBlackTreeNode* node) {
        return freebsd::RB_INSERT(m_impl.m_root, node, CompareImpl);
    }

    constexpr IntrusiveRedBlackTreeNode* FindImpl(IntrusiveRedBlackTreeNode const* node) const {
        return freebsd::RB_FIND(const_cast<ImplType::RootType&>(m_impl.m_root),
                                const_cast<IntrusiveRedBlackTreeNode*>(node), CompareImpl);
    }

    constexpr IntrusiveRedBlackTreeNode* NFindImpl(IntrusiveRedBlackTreeNode const* node) const {
        return freebsd::RB_NFIND(const_cast<ImplType::RootType&>(m_impl.m_root),
                                 const_cast<IntrusiveRedBlackTreeNode*>(node), CompareImpl);
    }

    constexpr IntrusiveRedBlackTreeNode* FindKeyImpl(const_key_reference key) const {
        return freebsd::RB_FIND_KEY(const_cast<ImplType::RootType&>(m_impl.m_root), key,
                                    CompareKeyImpl);
    }

    constexpr IntrusiveRedBlackTreeNode* NFindKeyImpl(const_key_reference key) const {
        return freebsd::RB_NFIND_KEY(const_cast<ImplType::RootType&>(m_impl.m_root), key,
                                     CompareKeyImpl);
    }

    constexpr IntrusiveRedBlackTreeNode* FindExistingImpl(
        IntrusiveRedBlackTreeNode const* node) const {
        return freebsd::RB_FIND_EXISTING(const_cast<ImplType::RootType&>(m_impl.m_root),
                                         const_cast<IntrusiveRedBlackTreeNode*>(node), CompareImpl);
    }

    constexpr IntrusiveRedBlackTreeNode* FindExistingKeyImpl(const_key_reference key) const {
        return freebsd::RB_FIND_EXISTING_KEY(const_cast<ImplType::RootType&>(m_impl.m_root), key,
                                             CompareKeyImpl);
    }

public:
    constexpr IntrusiveRedBlackTree() = default;

    // Iterator accessors.
    constexpr iterator begin() {
        return iterator(m_impl.begin());
    }

    constexpr const_iterator begin() const {
        return const_iterator(m_impl.begin());
    }

    constexpr iterator end() {
        return iterator(m_impl.end());
    }

    constexpr const_iterator end() const {
        return const_iterator(m_impl.end());
    }

    constexpr const_iterator cbegin() const {
        return this->begin();
    }

    constexpr const_iterator cend() const {
        return this->end();
    }

    constexpr iterator iterator_to(reference ref) {
        return iterator(m_impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
    }

    constexpr const_iterator iterator_to(const_reference ref) const {
        return const_iterator(m_impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
    }

    // Content management.
    constexpr bool empty() const {
        return m_impl.empty();
    }

    constexpr reference back() {
        return *Traits::GetParent(std::addressof(m_impl.back()));
    }

    constexpr const_reference back() const {
        return *Traits::GetParent(std::addressof(m_impl.back()));
    }

    constexpr reference front() {
        return *Traits::GetParent(std::addressof(m_impl.front()));
    }

    constexpr const_reference front() const {
        return *Traits::GetParent(std::addressof(m_impl.front()));
    }

    constexpr iterator erase(iterator it) {
        return iterator(m_impl.erase(it.GetImplIterator()));
    }

    constexpr iterator insert(reference ref) {
        ImplType::pointer node = Traits::GetNode(std::addressof(ref));
        this->InsertImpl(node);
        return iterator(node);
    }

    constexpr iterator find(const_reference ref) const {
        return iterator(this->FindImpl(Traits::GetNode(std::addressof(ref))));
    }

    constexpr iterator nfind(const_reference ref) const {
        return iterator(this->NFindImpl(Traits::GetNode(std::addressof(ref))));
    }

    constexpr iterator find_key(const_key_reference ref) const {
        return iterator(this->FindKeyImpl(ref));
    }

    constexpr iterator nfind_key(const_key_reference ref) const {
        return iterator(this->NFindKeyImpl(ref));
    }

    constexpr iterator find_existing(const_reference ref) const {
        return iterator(this->FindExistingImpl(Traits::GetNode(std::addressof(ref))));
    }

    constexpr iterator find_existing_key(const_key_reference ref) const {
        return iterator(this->FindExistingKeyImpl(ref));
    }
};

template <auto T, class Derived = Common::impl::GetParentType<T>>
class IntrusiveRedBlackTreeMemberTraits;

template <class Parent, IntrusiveRedBlackTreeNode Parent::*Member, class Derived>
class IntrusiveRedBlackTreeMemberTraits<Member, Derived> {
public:
    template <class Comparator>
    using TreeType = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeMemberTraits, Comparator>;
    using TreeTypeImpl = impl::IntrusiveRedBlackTreeImpl;

private:
    template <class, class, class>
    friend class IntrusiveRedBlackTree;

    friend class impl::IntrusiveRedBlackTreeImpl;

    static constexpr IntrusiveRedBlackTreeNode* GetNode(Derived* parent) {
        return std::addressof(parent->*Member);
    }

    static constexpr IntrusiveRedBlackTreeNode const* GetNode(Derived const* parent) {
        return std::addressof(parent->*Member);
    }

    static Derived* GetParent(IntrusiveRedBlackTreeNode* node) {
        return Common::GetParentPointer<Member, Derived>(node);
    }

    static Derived const* GetParent(IntrusiveRedBlackTreeNode const* node) {
        return Common::GetParentPointer<Member, Derived>(node);
    }
};

template <auto T, class Derived = Common::impl::GetParentType<T>>
class IntrusiveRedBlackTreeMemberTraitsDeferredAssert;

template <class Parent, IntrusiveRedBlackTreeNode Parent::*Member, class Derived>
class IntrusiveRedBlackTreeMemberTraitsDeferredAssert<Member, Derived> {
public:
    template <class Comparator>
    using TreeType =
        IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeMemberTraitsDeferredAssert, Comparator>;
    using TreeTypeImpl = impl::IntrusiveRedBlackTreeImpl;

private:
    template <class, class, class>
    friend class IntrusiveRedBlackTree;

    friend class impl::IntrusiveRedBlackTreeImpl;

    static constexpr IntrusiveRedBlackTreeNode* GetNode(Derived* parent) {
        return std::addressof(parent->*Member);
    }

    static constexpr IntrusiveRedBlackTreeNode const* GetNode(Derived const* parent) {
        return std::addressof(parent->*Member);
    }

    static Derived* GetParent(IntrusiveRedBlackTreeNode* node) {
        return Common::GetParentPointer<Member, Derived>(node);
    }

    static Derived const* GetParent(IntrusiveRedBlackTreeNode const* node) {
        return Common::GetParentPointer<Member, Derived>(node);
    }
};

template <class Derived>
class alignas(void*) IntrusiveRedBlackTreeBaseNode : public IntrusiveRedBlackTreeNode {
public:
    using IntrusiveRedBlackTreeNode::IntrusiveRedBlackTreeNode;

    constexpr Derived* GetPrev() {
        return static_cast<Derived*>(static_cast<IntrusiveRedBlackTreeBaseNode*>(
            impl::IntrusiveRedBlackTreeImpl::GetPrev(this)));
    }
    constexpr const Derived* GetPrev() const {
        return static_cast<const Derived*>(static_cast<const IntrusiveRedBlackTreeBaseNode*>(
            impl::IntrusiveRedBlackTreeImpl::GetPrev(this)));
    }

    constexpr Derived* GetNext() {
        return static_cast<Derived*>(static_cast<IntrusiveRedBlackTreeBaseNode*>(
            impl::IntrusiveRedBlackTreeImpl::GetNext(this)));
    }
    constexpr const Derived* GetNext() const {
        return static_cast<const Derived*>(static_cast<const IntrusiveRedBlackTreeBaseNode*>(
            impl::IntrusiveRedBlackTreeImpl::GetNext(this)));
    }
};

template <class Derived>
class IntrusiveRedBlackTreeBaseTraits {
public:
    template <class Comparator>
    using TreeType = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeBaseTraits, Comparator>;
    using TreeTypeImpl = impl::IntrusiveRedBlackTreeImpl;

private:
    template <class, class, class>
    friend class IntrusiveRedBlackTree;

    friend class impl::IntrusiveRedBlackTreeImpl;

    static constexpr IntrusiveRedBlackTreeNode* GetNode(Derived* parent) {
        return static_cast<IntrusiveRedBlackTreeNode*>(
            static_cast<IntrusiveRedBlackTreeBaseNode<Derived>*>(parent));
    }

    static constexpr IntrusiveRedBlackTreeNode const* GetNode(Derived const* parent) {
        return static_cast<const IntrusiveRedBlackTreeNode*>(
            static_cast<const IntrusiveRedBlackTreeBaseNode<Derived>*>(parent));
    }

    static constexpr Derived* GetParent(IntrusiveRedBlackTreeNode* node) {
        return static_cast<Derived*>(static_cast<IntrusiveRedBlackTreeBaseNode<Derived>*>(node));
    }

    static constexpr Derived const* GetParent(IntrusiveRedBlackTreeNode const* node) {
        return static_cast<const Derived*>(
            static_cast<const IntrusiveRedBlackTreeBaseNode<Derived>*>(node));
    }
};

} // namespace Common
