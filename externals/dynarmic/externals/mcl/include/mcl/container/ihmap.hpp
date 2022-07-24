// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include "mcl/assert.hpp"
#include "mcl/container/detail/meta_byte.hpp"
#include "mcl/container/detail/meta_byte_group.hpp"
#include "mcl/container/detail/slot_union.hpp"
#include "mcl/hash/xmrx.hpp"
#include "mcl/hint/assume.hpp"

namespace mcl {

template<typename KeyType, typename MappedType, typename Hash, typename Pred>
class ihmap;

namespace detail {

constexpr std::array<meta_byte, 16> ihmap_default_meta{
    meta_byte::empty, meta_byte::empty, meta_byte::empty, meta_byte::empty,
    meta_byte::empty, meta_byte::empty, meta_byte::empty, meta_byte::empty,
    meta_byte::empty, meta_byte::empty, meta_byte::empty, meta_byte::empty,
    meta_byte::empty, meta_byte::empty, meta_byte::empty, meta_byte::tombstone};

template<typename KeyType, typename MappedType>
struct ihmap_group {
    using base_value_type = std::pair<const KeyType, MappedType>;
    using slot_type = detail::slot_union<base_value_type>;

    static constexpr std::size_t group_size{meta_byte_group::max_group_size - 1};

    meta_byte_group meta{ihmap_default_meta};
    std::array<slot_type, group_size> slots{};
};

}  // namespace detail

template<bool IsConst, typename KeyType, typename MappedType, typename Hash, typename Pred>
class ihmap_iterator {
    using group_type = detail::ihmap_group<KeyType, MappedType>;
    using base_value_type = typename group_type::base_value_type;

public:
    using key_type = KeyType;
    using mapped_type = MappedType;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::conditional_t<IsConst, std::add_const_t<base_value_type>, base_value_type>;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    ihmap_iterator() = default;
    ihmap_iterator(const ihmap_iterator& other) = default;
    ihmap_iterator& operator=(const ihmap_iterator& other) = default;

    ihmap_iterator& operator++()
    {
        if (group_ptr == nullptr)
            return *this;

        ++slot_index;

        skip_empty_or_tombstone();

        return *this;
    }
    ihmap_iterator operator++(int)
    {
        ihmap_iterator it(*this);
        ++*this;
        return it;
    }

    bool operator==(const ihmap_iterator& other) const
    {
        return std::tie(group_ptr, slot_index) == std::tie(other.group_ptr, other.slot_index);
    }
    bool operator!=(const ihmap_iterator& other) const
    {
        return !operator==(other);
    }

    reference operator*() const
    {
        return static_cast<reference>(group_ptr->slots[slot_index].value);
    }
    pointer operator->() const
    {
        return std::addressof(operator*());
    }

private:
    friend class ihmap<KeyType, MappedType, Hash, Pred>;

    ihmap_iterator(group_type* group_ptr, size_t slot_index)
        : group_ptr{group_ptr}, slot_index{slot_index}
    {
        ASSUME(group_ptr != nullptr);
    }

    void skip_empty_or_tombstone()
    {
        if (!group_ptr)
            return;

        while (true) {
            const detail::meta_byte mb = group_ptr->meta.get(slot_index);
            if (slot_index == group_type::group_size) {
                slot_index = 0;
                ++group_ptr;

                if (mb == detail::meta_byte::end_sentinel) {
                    group_ptr = nullptr;
                    return;
                }

                continue;
            }
            if (is_full(mb)) {
                break;
            }
            ++slot_index;
        }
    }

    group_type* group_ptr{nullptr};
    std::size_t slot_index{0};
};

template<typename KeyType, typename MappedType, typename Hash = hash::avalanche_xmrx<KeyType>, typename Pred = std::equal_to<KeyType>>
class ihmap {
    using group_type = detail::ihmap_group<KeyType, MappedType>;

public:
    using key_type = KeyType;
    using mapped_type = MappedType;
    using hasher = Hash;
    using key_equal = Pred;
    using value_type = typename group_type::base_value_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator = ihmap_iterator<false, key_type, mapped_type, hasher, key_equal>;
    using const_iterator = ihmap_iterator<true, key_type, mapped_type, hasher, key_equal>;

private:
    static_assert(!std::is_reference_v<key_type>);
    static_assert(!std::is_reference_v<mapped_type>);

    static constexpr std::size_t group_size{group_type::group_size};
    static constexpr std::size_t average_max_group_load{group_size - 2};

    struct position {
        std::size_t group_index;
        std::size_t slot_index;
    };

public:
    ihmap()
    {
        initialize_members(1);
    }
    ihmap(const ihmap& other)
    {
        deep_copy(other);
    }
    ihmap(ihmap&& other)
        : group_index_mask{std::exchange(other.group_index_mask, 0)}
        , empty_slots{std::exchange(other.empty_slots, 0)}
        , full_slots{std::exchange(other.full_slots, 0)}
        , groups{std::move(other.groups)}
    {
    }
    ihmap& operator=(const ihmap& other)
    {
        deep_copy(other);
        return *this;
    }
    ihmap& operator=(ihmap&& other)
    {
        group_index_mask = std::exchange(other.group_index_mask, 0);
        empty_slots = std::exchange(other.empty_slots, 0);
        full_slots = std::exchange(other.full_slots, 0);
        groups = std::move(other.groups);
        return *this;
    }

    ~ihmap()
    {
        if (!groups)
            return;

        clear();
    }

    [[nodiscard]] bool empty() const noexcept { return full_slots == 0; }
    size_type size() const noexcept { return full_slots; }
    size_type max_size() const noexcept { return static_cast<size_type>(std::numeric_limits<difference_type>::max()); }

    iterator begin()
    {
        iterator result{iterator_at({0, 0})};
        result.skip_empty_or_tombstone();
        return result;
    }
    iterator end()
    {
        return {};
    }
    const_iterator cbegin() const
    {
        const_iterator result{const_iterator_at({0, 0})};
        result.skip_empty_or_tombstone();
        return result;
    }
    const_iterator cend() const
    {
        return {};
    }
    const_iterator begin() const
    {
        return cbegin();
    }
    const_iterator end() const
    {
        return cend();
    }

    template<typename K = key_type, typename... Args>
    std::pair<iterator, bool> try_emplace(K&& k, Args&&... args)
    {
        auto [pos, item_found] = find_key_or_empty_slot(k);
        if (!item_found) {
            new (&groups[pos.group_index].slots[pos.slot_index].value) value_type(
                std::piecewise_construct,
                std::forward_as_tuple(std::forward<K>(k)),
                std::forward_as_tuple(std::forward<Args>(args)...));
        }
        return {iterator_at(pos), !item_found};
    }

    template<typename K = key_type, typename V = mapped_type>
    std::pair<iterator, bool> insert_or_assign(K&& k, V&& v)
    {
        auto [pos, item_found] = find_key_or_empty_slot(k);
        if (item_found) {
            groups[pos.group_index].slots[pos.slot_index].value.second = std::forward<V>(v);
        } else {
            new (&groups[pos.group_index].slots[pos.slot_index].value) value_type(
                std::forward<K>(k),
                std::forward<V>(v));
        }
        return {iterator_at(pos), !item_found};
    }

    void erase(const_iterator iter)
    {
        if (iter == cend()) {
            return;
        }

        const std::size_t group_index{static_cast<std::size_t>(std::distance(groups.get(), iter.group_ptr))};

        erase_impl({group_index, iter.slot_index});
    }
    void erase(iterator iter)
    {
        if (iter == end()) {
            return;
        }

        const std::size_t group_index{static_cast<std::size_t>(std::distance(groups.get(), iter.group_ptr))};

        erase_impl({group_index, iter.slot_index});
    }
    template<typename K = key_type>
    std::size_t erase(const K& key)
    {
        const std::size_t hash{hasher{}(key)};
        const detail::meta_byte mb{detail::meta_byte_from_hash(hash)};

        std::size_t group_index{detail::group_index_from_hash(hash, group_index_mask)};

        while (true) {
            const group_type& g{groups[group_index]};

            MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(g.meta.match(mb), {
                if (key_equal{}(g.slots[match_index].value.first, key)) [[likely]] {
                    erase_impl({group_index, match_index});
                    return 1;
                }
            });

            if (g.meta.is_any_empty()) [[likely]] {
                return 0;
            }

            group_index = (group_index + 1) & group_index_mask;
        }
    }

    template<typename K = key_type>
    iterator find(const K& key)
    {
        const std::size_t hash{hasher{}(key)};
        const detail::meta_byte mb{detail::meta_byte_from_hash(hash)};

        std::size_t group_index{detail::group_index_from_hash(hash, group_index_mask)};

        while (true) {
            const group_type& g{groups[group_index]};

            MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(g.meta.match(mb), {
                if (key_equal{}(g.slots[match_index].value.first, key)) [[likely]] {
                    return iterator_at({group_index, match_index});
                }
            });

            if (g.meta.is_any_empty()) [[likely]] {
                return {};
            }

            group_index = (group_index + 1) & group_index_mask;
        }
    }
    template<typename K = key_type>
    const_iterator find(const K& key) const
    {
        const std::size_t hash{hasher{}(key)};
        const detail::meta_byte mb{detail::meta_byte_from_hash(hash)};

        std::size_t group_index{detail::group_index_from_hash(hash, group_index_mask)};

        while (true) {
            const group_type& g{groups[group_index]};

            MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(g.meta.match(mb), {
                if (key_equal{}(g.slots[match_index].value.first, key)) [[likely]] {
                    return const_iterator_at({group_index, match_index});
                }
            });

            if (g.meta.is_any_empty()) [[likely]] {
                return {};
            }

            group_index = (group_index + 1) & group_index_mask;
        }
    }
    template<typename K = key_type>
    bool contains(const K& key) const
    {
        return find(key) != end();
    }
    template<typename K = key_type>
    std::size_t count(const K& key) const
    {
        return contains(key) ? 1 : 0;
    }

    template<typename K = key_type>
    mapped_type& operator[](K&& k)
    {
        return try_emplace(std::forward<K>(k)).first->second;
    }
    template<typename K = key_type>
    mapped_type& at(K&& k)
    {
        const auto iter{find(k)};
        if (iter == end()) {
            throw std::out_of_range("ihmap::at: key not found");
        }
        return iter->second;
    }
    template<typename K = key_type>
    const mapped_type& at(K&& k) const
    {
        const auto iter{find(k)};
        if (iter == end()) {
            throw std::out_of_range("ihmap::at: key not found");
        }
        return iter->second;
    }

    void clear()
    {
        for (auto iter{begin()}; iter != end(); ++iter) {
            iter->~value_type();
        }

        clear_metadata();
    }

private:
    iterator iterator_at(position pos)
    {
        return {groups.get() + pos.group_index, pos.slot_index};
    }
    const_iterator const_iterator_at(position pos) const
    {
        return {groups.get() + pos.group_index, pos.slot_index};
    }

    std::pair<position, bool> find_key_or_empty_slot(const key_type& key)
    {
        const std::size_t hash{hasher{}(key)};
        const detail::meta_byte mb{detail::meta_byte_from_hash(hash)};

        std::size_t group_index{detail::group_index_from_hash(hash, group_index_mask)};

        while (true) {
            const group_type& g{groups[group_index]};

            MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(g.meta.match(mb), {
                if (key_equal{}(g.slots[match_index].value.first, key)) [[likely]] {
                    return {{group_index, match_index}, true};
                }
            });

            if (g.meta.is_any_empty()) [[likely]] {
                return {find_empty_slot_to_insert(hash), false};
            }

            group_index = (group_index + 1) & group_index_mask;
        }
    }

    position find_empty_slot_to_insert(const std::size_t hash)
    {
        if (empty_slots == 0) [[unlikely]] {
            grow_and_rehash();
        }

        std::size_t group_index{detail::group_index_from_hash(hash, group_index_mask)};

        while (true) {
            group_type& g{groups[group_index]};

            MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(g.meta.match_empty_or_tombstone(), {
                if (g.meta.get(match_index) == detail::meta_byte::empty) [[likely]] {
                    --empty_slots;
                }
                ++full_slots;

                g.meta.set(match_index, detail::meta_byte_from_hash(hash));

                return {group_index, match_index};
            });

            group_index = (group_index + 1) & group_index_mask;
        }
    }

    void erase_impl(position pos)
    {
        group_type& g{groups[pos.group_index]};

        g.slots[pos.slot_index].value.~value_type();

        --full_slots;
        if (g.meta.is_any_empty()) {
            g.meta.set(pos.slot_index, detail::meta_byte::empty);
            ++empty_slots;
        } else {
            g.meta.set(pos.slot_index, detail::meta_byte::tombstone);
        }
    }

    void grow_and_rehash()
    {
        const std::size_t new_group_count{2 * (group_index_mask + 1)};

        pow2_resize(new_group_count);
    }

    void pow2_resize(std::size_t new_group_count)
    {
        auto iter{begin()};

        const auto old_groups{std::move(groups)};

        initialize_members(new_group_count);

        for (; iter != end(); ++iter) {
            const std::size_t hash{hasher{}(iter->first)};
            const position pos{find_empty_slot_to_insert(hash)};

            new (&groups[pos.group_index].slots[pos.slot_index].value) value_type(std::move(iter.group_ptr->slots[iter.slot_index].value));
            iter.group_ptr->slots[iter.slot_index].value.~value_type();
        }
    }

    void deep_copy(const ihmap& other)
    {
        initialize_members(other.group_index_mask + 1);

        for (auto iter = other.begin(); iter != other.end(); ++iter) {
            const std::size_t hash{hasher{}(iter->first)};
            const position pos{find_empty_slot_to_insert(hash)};

            new (&groups[pos.group_index].slots[pos.slot_index].value) value_type(iter.group_ptr->slots[iter.slot_index].value);
        }
    }

    void initialize_members(std::size_t group_count)
    {
        // DEBUG_ASSERT(group_count != 0 && std::ispow2(group_count));

        group_index_mask = group_count - 1;
        groups = std::unique_ptr<group_type[]>{new group_type[group_count]};

        clear_metadata();
    }

    void clear_metadata()
    {
        const std::size_t group_count{group_index_mask + 1};

        empty_slots = group_count * average_max_group_load;
        full_slots = 0;

        for (size_t i{0}; i < group_count; ++i) {
            groups[i].meta = detail::meta_byte_group{detail::ihmap_default_meta};
        }
        groups[group_count - 1].meta.set(group_size, detail::meta_byte::end_sentinel);
    }

    std::size_t group_index_mask;
    std::size_t empty_slots;
    std::size_t full_slots;
    std::unique_ptr<group_type[]> groups;
};

}  // namespace mcl
