// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include <type_traits>

#include "common/common_types.h"

namespace Common {

template <typename KeyTBase, typename ValueT>
class RangeMap {
private:
    using KeyT =
        std::conditional_t<std::is_signed_v<KeyTBase>, KeyTBase, std::make_signed_t<KeyTBase>>;

public:
    explicit RangeMap(ValueT null_value_) : null_value{null_value_} {
        container.emplace(std::numeric_limits<KeyT>::min(), null_value);
    };
    ~RangeMap() = default;

    void Map(KeyTBase address, KeyTBase address_end, ValueT value) {
        KeyT new_address = static_cast<KeyT>(address);
        KeyT new_address_end = static_cast<KeyT>(address_end);
        if (new_address < 0) {
            new_address = 0;
        }
        if (new_address_end < 0) {
            new_address_end = 0;
        }
        InternalMap(new_address, new_address_end, value);
    }

    void Unmap(KeyTBase address, KeyTBase address_end) {
        Map(address, address_end, null_value);
    }

    [[nodiscard]] size_t GetContinuousSizeFrom(KeyTBase address) const {
        const KeyT new_address = static_cast<KeyT>(address);
        if (new_address < 0) {
            return 0;
        }
        return ContinuousSizeInternal(new_address);
    }

    [[nodiscard]] ValueT GetValueAt(KeyT address) const {
        const KeyT new_address = static_cast<KeyT>(address);
        if (new_address < 0) {
            return null_value;
        }
        return GetValueInternal(new_address);
    }

private:
    using MapType = std::map<KeyT, ValueT>;
    using IteratorType = typename MapType::iterator;
    using ConstIteratorType = typename MapType::const_iterator;

    size_t ContinuousSizeInternal(KeyT address) const {
        const auto it = GetFirstElementBeforeOrOn(address);
        if (it == container.end() || it->second == null_value) {
            return 0;
        }
        const auto it_end = std::next(it);
        if (it_end == container.end()) {
            return std::numeric_limits<KeyT>::max() - address;
        }
        return it_end->first - address;
    }

    ValueT GetValueInternal(KeyT address) const {
        const auto it = GetFirstElementBeforeOrOn(address);
        if (it == container.end()) {
            return null_value;
        }
        return it->second;
    }

    ConstIteratorType GetFirstElementBeforeOrOn(KeyT address) const {
        auto it = container.lower_bound(address);
        if (it == container.begin()) {
            return it;
        }
        if (it != container.end() && (it->first == address)) {
            return it;
        }
        --it;
        return it;
    }

    ValueT GetFirstValueWithin(KeyT address) {
        auto it = container.lower_bound(address);
        if (it == container.begin()) {
            return it->second;
        }
        if (it == container.end()) [[unlikely]] { // this would be a bug
            return null_value;
        }
        --it;
        return it->second;
    }

    ValueT GetLastValueWithin(KeyT address) {
        auto it = container.upper_bound(address);
        if (it == container.end()) {
            return null_value;
        }
        if (it == container.begin()) [[unlikely]] { // this would be a bug
            return it->second;
        }
        --it;
        return it->second;
    }

    void InternalMap(KeyT address, KeyT address_end, ValueT value) {
        const bool must_add_start = GetFirstValueWithin(address) != value;
        const ValueT last_value = GetLastValueWithin(address_end);
        const bool must_add_end = last_value != value;
        auto it = container.lower_bound(address);
        const auto it_end = container.upper_bound(address_end);
        while (it != it_end) {
            it = container.erase(it);
        }
        if (must_add_start) {
            container.emplace(address, value);
        }
        if (must_add_end) {
            container.emplace(address_end, last_value);
        }
    }

    ValueT null_value;
    MapType container;
};

} // namespace Common
