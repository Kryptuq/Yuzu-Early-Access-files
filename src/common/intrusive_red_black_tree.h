// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/parent_of_member.h"
#include "common/tree.h"

namespace Common {

namespace impl {

class IntrusiveRedBlackTreeImpl;

}

struct IntrusiveRedBlackTreeNode {
public:
    using EntryType = RBEntry<IntrusiveRedBlackTreeNode>;

    constexpr IntrusiveRedBlackTreeNode() = default;

    void SetEntry(const EntryType& new_entry) {
        entry = new_entry;
    }

    [[nodiscard]] EntryType& GetEntry() {
        return entry;
    }

    [[nodiscard]] const EntryType& GetEntry() const {
        return entry;
    }

private:
    EntryType entry{};

    friend class impl::IntrusiveRedBlackTreeImpl;

    template <class, class, class>
    friend class IntrusiveRedBlackTree;
};

template <class T, class Traits, class Comparator>
class IntrusiveRedBlackTree;

namespace impl {

class IntrusiveRedBlackTreeImpl {
private:
    template <class, class, class>
    friend class ::Common::IntrusiveRedBlackTree;

    using RootType = RBHead<IntrusiveRedBlackTreeNode>;
    RootType root;

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
        using pointer = std::conditional_t<Const, IntrusiveRedBlackTreeImpl::const_pointer,
                                           IntrusiveRedBlackTreeImpl::pointer>;
        using reference = std::conditional_t<Const, IntrusiveRedBlackTreeImpl::const_reference,
                                             IntrusiveRedBlackTreeImpl::reference>;

    private:
        pointer node;

    public:
        explicit Iterator(pointer n) : node(n) {}

        bool operator==(const Iterator& rhs) const {
            return this->node == rhs.node;
        }

        bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }

        pointer operator->() const {
            return this->node;
        }

        reference operator*() const {
            return *this->node;
        }

        Iterator& operator++() {
            this->node = GetNext(this->node);
            return *this;
        }

        Iterator& operator--() {
            this->node = GetPrev(this->node);
            return *this;
        }

        Iterator operator++(int) {
            const Iterator it{*this};
            ++(*this);
            return it;
        }

        Iterator operator--(int) {
            const Iterator it{*this};
            --(*this);
            return it;
        }

        operator Iterator<true>() const {
            return Iterator<true>(this->node);
        }
    };

private:
    // Define accessors using RB_* functions.
    bool EmptyImpl() const {
        return root.IsEmpty();
    }

    IntrusiveRedBlackTreeNode* GetMinImpl() const {
        return RB_MIN(const_cast<RootType*>(&root));
    }

    IntrusiveRedBlackTreeNode* GetMaxImpl() const {
        return RB_MAX(const_cast<RootType*>(&root));
    }

    IntrusiveRedBlackTreeNode* RemoveImpl(IntrusiveRedBlackTreeNode* node) {
        return RB_REMOVE(&root, node);
    }

public:
    static IntrusiveRedBlackTreeNode* GetNext(IntrusiveRedBlackTreeNode* node) {
        return RB_NEXT(node);
    }

    static IntrusiveRedBlackTreeNode* GetPrev(IntrusiveRedBlackTreeNode* node) {
        return RB_PREV(node);
    }

    static const IntrusiveRedBlackTreeNode* GetNext(const IntrusiveRedBlackTreeNode* node) {
        return static_cast<const IntrusiveRedBlackTreeNode*>(
            GetNext(const_cast<IntrusiveRedBlackTreeNode*>(node)));
    }

    static const IntrusiveRedBlackTreeNode* GetPrev(const IntrusiveRedBlackTreeNode* node) {
        return static_cast<const IntrusiveRedBlackTreeNode*>(
            GetPrev(const_cast<IntrusiveRedBlackTreeNode*>(node)));
    }

public:
    constexpr IntrusiveRedBlackTreeImpl() {}

    // Iterator accessors.
    iterator begin() {
        return iterator(this->GetMinImpl());
    }

    const_iterator begin() const {
        return const_iterator(this->GetMinImpl());
    }

    iterator end() {
        return iterator(static_cast<IntrusiveRedBlackTreeNode*>(nullptr));
    }

    const_iterator end() const {
        return const_iterator(static_cast<const IntrusiveRedBlackTreeNode*>(nullptr));
    }

    const_iterator cbegin() const {
        return this->begin();
    }

    const_iterator cend() const {
        return this->end();
    }

    iterator iterator_to(reference ref) {
        return iterator(&ref);
    }

    const_iterator iterator_to(const_reference ref) const {
        return const_iterator(&ref);
    }

    // Content management.
    bool empty() const {
        return this->EmptyImpl();
    }

    reference back() {
        return *this->GetMaxImpl();
    }

    const_reference back() const {
        return *this->GetMaxImpl();
    }

    reference front() {
        return *this->GetMinImpl();
    }

    const_reference front() const {
        return *this->GetMinImpl();
    }

    iterator erase(iterator it) {
        auto cur = std::addressof(*it);
        auto next = GetNext(cur);
        this->RemoveImpl(cur);
        return iterator(next);
    }
};

} // namespace impl

template <typename T>
concept HasLightCompareType = requires {
    { std::is_same<typename T::LightCompareType, void>::value } -> std::convertible_to<bool>;
};

namespace impl {

    template <typename T, typename Default>
    consteval auto* GetLightCompareType() {
        if constexpr (HasLightCompareType<T>) {
            return static_cast<typename T::LightCompareType*>(nullptr);
        } else {
            return static_cast<Default*>(nullptr);
        }
    }

} // namespace impl

template <typename T, typename Default>
using LightCompareType = std::remove_pointer_t<decltype(impl::GetLightCompareType<T, Default>())>;

template <class T, class Traits, class Comparator>
class IntrusiveRedBlackTree {

public:
    using ImplType = impl::IntrusiveRedBlackTreeImpl;

private:
    ImplType impl{};

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

    using light_value_type = LightCompareType<Comparator, value_type>;
    using const_light_pointer = const light_value_type*;
    using const_light_reference = const light_value_type&;

    template <bool Const>
    class Iterator {
    public:
        friend class IntrusiveRedBlackTree<T, Traits, Comparator>;

        using ImplIterator =
            std::conditional_t<Const, ImplType::const_iterator, ImplType::iterator>;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename IntrusiveRedBlackTree::value_type;
        using difference_type = typename IntrusiveRedBlackTree::difference_type;
        using pointer = std::conditional_t<Const, IntrusiveRedBlackTree::const_pointer,
                                           IntrusiveRedBlackTree::pointer>;
        using reference = std::conditional_t<Const, IntrusiveRedBlackTree::const_reference,
                                             IntrusiveRedBlackTree::reference>;

    private:
        ImplIterator iterator;

    private:
        explicit Iterator(ImplIterator it) : iterator(it) {}

        explicit Iterator(typename std::conditional<Const, ImplType::const_iterator,
                                                    ImplType::iterator>::type::pointer ptr)
            : iterator(ptr) {}

        ImplIterator GetImplIterator() const {
            return this->iterator;
        }

    public:
        bool operator==(const Iterator& rhs) const {
            return this->iterator == rhs.iterator;
        }

        bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }

        pointer operator->() const {
            return Traits::GetParent(std::addressof(*this->iterator));
        }

        reference operator*() const {
            return *Traits::GetParent(std::addressof(*this->iterator));
        }

        Iterator& operator++() {
            ++this->iterator;
            return *this;
        }

        Iterator& operator--() {
            --this->iterator;
            return *this;
        }

        Iterator operator++(int) {
            const Iterator it{*this};
            ++this->iterator;
            return it;
        }

        Iterator operator--(int) {
            const Iterator it{*this};
            --this->iterator;
            return it;
        }

        operator Iterator<true>() const {
            return Iterator<true>(this->iterator);
        }
    };

private:
    static int CompareImpl(const IntrusiveRedBlackTreeNode* lhs,
                           const IntrusiveRedBlackTreeNode* rhs) {
        return Comparator::Compare(*Traits::GetParent(lhs), *Traits::GetParent(rhs));
    }

    static int LightCompareImpl(const void* elm, const IntrusiveRedBlackTreeNode* rhs) {
        return Comparator::Compare(*static_cast<const_light_pointer>(elm), *Traits::GetParent(rhs));
    }

    // Define accessors using RB_* functions.
    IntrusiveRedBlackTreeNode* InsertImpl(IntrusiveRedBlackTreeNode* node) {
        return RB_INSERT(&impl.root, node, CompareImpl);
    }

    IntrusiveRedBlackTreeNode* FindImpl(const IntrusiveRedBlackTreeNode* node) const {
        return RB_FIND(const_cast<ImplType::RootType*>(&impl.root),
                       const_cast<IntrusiveRedBlackTreeNode*>(node), CompareImpl);
    }

    IntrusiveRedBlackTreeNode* NFindImpl(const IntrusiveRedBlackTreeNode* node) const {
        return RB_NFIND(const_cast<ImplType::RootType*>(&impl.root),
                        const_cast<IntrusiveRedBlackTreeNode*>(node), CompareImpl);
    }

    IntrusiveRedBlackTreeNode* FindLightImpl(const_light_pointer lelm) const {
        return RB_FIND_LIGHT(const_cast<ImplType::RootType*>(&impl.root),
                             static_cast<const void*>(lelm), LightCompareImpl);
    }

    IntrusiveRedBlackTreeNode* NFindLightImpl(const_light_pointer lelm) const {
        return RB_NFIND_LIGHT(const_cast<ImplType::RootType*>(&impl.root),
                              static_cast<const void*>(lelm), LightCompareImpl);
    }

public:
    constexpr IntrusiveRedBlackTree() = default;

    // Iterator accessors.
    iterator begin() {
        return iterator(this->impl.begin());
    }

    const_iterator begin() const {
        return const_iterator(this->impl.begin());
    }

    iterator end() {
        return iterator(this->impl.end());
    }

    const_iterator end() const {
        return const_iterator(this->impl.end());
    }

    const_iterator cbegin() const {
        return this->begin();
    }

    const_iterator cend() const {
        return this->end();
    }

    iterator iterator_to(reference ref) {
        return iterator(this->impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
    }

    const_iterator iterator_to(const_reference ref) const {
        return const_iterator(this->impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
    }

    // Content management.
    bool empty() const {
        return this->impl.empty();
    }

    reference back() {
        return *Traits::GetParent(std::addressof(this->impl.back()));
    }

    const_reference back() const {
        return *Traits::GetParent(std::addressof(this->impl.back()));
    }

    reference front() {
        return *Traits::GetParent(std::addressof(this->impl.front()));
    }

    const_reference front() const {
        return *Traits::GetParent(std::addressof(this->impl.front()));
    }

    iterator erase(iterator it) {
        return iterator(this->impl.erase(it.GetImplIterator()));
    }

    iterator insert(reference ref) {
        ImplType::pointer node = Traits::GetNode(std::addressof(ref));
        this->InsertImpl(node);
        return iterator(node);
    }

    iterator find(const_reference ref) const {
        return iterator(this->FindImpl(Traits::GetNode(std::addressof(ref))));
    }

    iterator nfind(const_reference ref) const {
        return iterator(this->NFindImpl(Traits::GetNode(std::addressof(ref))));
    }

    iterator find_light(const_light_reference ref) const {
        return iterator(this->FindLightImpl(std::addressof(ref)));
    }

    iterator nfind_light(const_light_reference ref) const {
        return iterator(this->NFindLightImpl(std::addressof(ref)));
    }
};

template <auto T, class Derived = impl::GetParentType<T>>
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

    static constexpr Derived* GetParent(IntrusiveRedBlackTreeNode* node) {
        return GetParentPointer<Member, Derived>(node);
    }

    static constexpr Derived const* GetParent(const IntrusiveRedBlackTreeNode* node) {
        return GetParentPointer<Member, Derived>(node);
    }

private:
    static constexpr TypedStorage<Derived> DerivedStorage = {};
};

template <auto T, class Derived = impl::GetParentType<T>>
class IntrusiveRedBlackTreeMemberTraitsDeferredAssert;

template <class Parent, IntrusiveRedBlackTreeNode Parent::*Member, class Derived>
class IntrusiveRedBlackTreeMemberTraitsDeferredAssert<Member, Derived> {
public:
    template <class Comparator>
    using TreeType =
        IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeMemberTraitsDeferredAssert, Comparator>;
    using TreeTypeImpl = impl::IntrusiveRedBlackTreeImpl;

    static constexpr bool IsValid() {
        TypedStorage<Derived> DerivedStorage = {};
        return GetParent(GetNode(GetPointer(DerivedStorage))) == GetPointer(DerivedStorage);
    }

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

    static constexpr Derived* GetParent(IntrusiveRedBlackTreeNode* node) {
        return GetParentPointer<Member, Derived>(node);
    }

    static constexpr Derived const* GetParent(const IntrusiveRedBlackTreeNode* node) {
        return GetParentPointer<Member, Derived>(node);
    }
};

template <class Derived>
class IntrusiveRedBlackTreeBaseNode : public IntrusiveRedBlackTreeNode {
public:
    constexpr Derived* GetPrev() {
        return static_cast<Derived*>(impl::IntrusiveRedBlackTreeImpl::GetPrev(this));
    }
    constexpr const Derived* GetPrev() const {
        return static_cast<const Derived*>(impl::IntrusiveRedBlackTreeImpl::GetPrev(this));
    }

    constexpr Derived* GetNext() {
        return static_cast<Derived*>(impl::IntrusiveRedBlackTreeImpl::GetNext(this));
    }
    constexpr const Derived* GetNext() const {
        return static_cast<const Derived*>(impl::IntrusiveRedBlackTreeImpl::GetNext(this));
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
        return static_cast<IntrusiveRedBlackTreeNode*>(parent);
    }

    static constexpr IntrusiveRedBlackTreeNode const* GetNode(Derived const* parent) {
        return static_cast<const IntrusiveRedBlackTreeNode*>(parent);
    }

    static constexpr Derived* GetParent(IntrusiveRedBlackTreeNode* node) {
        return static_cast<Derived*>(node);
    }

    static constexpr Derived const* GetParent(const IntrusiveRedBlackTreeNode* node) {
        return static_cast<const Derived*>(node);
    }
};

} // namespace Common
