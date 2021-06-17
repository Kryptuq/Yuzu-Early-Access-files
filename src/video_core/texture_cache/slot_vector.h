// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <bit>
#include <concepts>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

#include "common/assert.h"
#include "common/common_types.h"

namespace VideoCommon {

struct SlotId {
    static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();

    constexpr auto operator<=>(const SlotId&) const noexcept = default;

    constexpr explicit operator bool() const noexcept {
        return index != INVALID_INDEX;
    }

    u32 index = INVALID_INDEX;
};

template <class T>
requires std::is_nothrow_move_assignable_v<T>&&
    std::is_nothrow_move_constructible_v<T> class SlotVector {
public:
    class Iterator {
        friend SlotVector<T>;

    public:
        constexpr Iterator() = default;

        Iterator& operator++() noexcept {
            const u64* const bitset = slot_vector->stored_bitset.data();
            const u32 size = static_cast<u32>(slot_vector->stored_bitset.size()) * 64;
            if (id.index < size) {
                do {
                    ++id.index;
                } while (id.index < size && !IsValid(bitset));
                if (id.index == size) {
                    id.index = SlotId::INVALID_INDEX;
                }
            }
            return *this;
        }

        Iterator operator++(int) noexcept {
            const Iterator copy{*this};
            ++*this;
            return copy;
        }

        bool operator==(const Iterator& other) const noexcept {
            return id.index == other.id.index;
        }

        bool operator!=(const Iterator& other) const noexcept {
            return id.index != other.id.index;
        }

        std::pair<SlotId, T*> operator*() const noexcept {
            return {id, std::addressof((*slot_vector)[id])};
        }

        T* operator->() const noexcept {
            return std::addressof((*slot_vector)[id]);
        }

    private:
        Iterator(SlotVector<T>* slot_vector_, SlotId id_) noexcept
            : slot_vector{slot_vector_}, id{id_} {}

        bool IsValid(const u64* bitset) const noexcept {
            return ((bitset[id.index / 64] >> (id.index % 64)) & 1) != 0;
        }

        SlotVector<T>* slot_vector;
        SlotId id;
    };

    ~SlotVector() noexcept {
        size_t index = 0;
        for (u64 bits : stored_bitset) {
            for (size_t bit = 0; bits; ++bit, bits >>= 1) {
                if ((bits & 1) != 0) {
                    values[index + bit].object.~T();
                }
            }
            index += 64;
        }
        delete[] values;
    }

    [[nodiscard]] T& operator[](SlotId id) noexcept {
        ValidateIndex(id);
        return values[id.index].object;
    }

    [[nodiscard]] const T& operator[](SlotId id) const noexcept {
        ValidateIndex(id);
        return values[id.index].object;
    }

    template <typename... Args>
    [[nodiscard]] SlotId insert(Args&&... args) noexcept {
        const u32 index = FreeValueIndex();
        new (&values[index].object) T(std::forward<Args>(args)...);
        SetStorageBit(index);

        return SlotId{index};
    }

    void erase(SlotId id) noexcept {
        values[id.index].object.~T();
        free_list.push_back(id.index);
        ResetStorageBit(id.index);
    }

    [[nodiscard]] Iterator begin() noexcept {
        const auto it = std::ranges::find_if(stored_bitset, [](u64 value) { return value != 0; });
        if (it == stored_bitset.end()) {
            return end();
        }
        const u32 word_index = static_cast<u32>(std::distance(it, stored_bitset.begin()));
        const SlotId first_id{word_index * 64 + static_cast<u32>(std::countr_zero(*it))};
        return Iterator(this, first_id);
    }

    [[nodiscard]] Iterator end() noexcept {
        return Iterator(this, SlotId{SlotId::INVALID_INDEX});
    }

private:
    struct NonTrivialDummy {
        NonTrivialDummy() noexcept {}
    };

    union Entry {
        Entry() noexcept : dummy{} {}
        ~Entry() noexcept {}

        NonTrivialDummy dummy;
        T object;
    };

    void SetStorageBit(u32 index) noexcept {
        stored_bitset[index / 64] |= u64(1) << (index % 64);
    }

    void ResetStorageBit(u32 index) noexcept {
        stored_bitset[index / 64] &= ~(u64(1) << (index % 64));
    }

    bool ReadStorageBit(u32 index) noexcept {
        return ((stored_bitset[index / 64] >> (index % 64)) & 1) != 0;
    }

    void ValidateIndex(SlotId id) const noexcept {
        DEBUG_ASSERT(id);
        DEBUG_ASSERT(id.index / 64 < stored_bitset.size());
        DEBUG_ASSERT(((stored_bitset[id.index / 64] >> (id.index % 64)) & 1) != 0);
    }

    [[nodiscard]] u32 FreeValueIndex() noexcept {
        if (free_list.empty()) {
            Reserve(values_capacity ? (values_capacity << 1) : 1);
        }
        const u32 free_index = free_list.back();
        free_list.pop_back();
        return free_index;
    }

    void Reserve(size_t new_capacity) noexcept {
        Entry* const new_values = new Entry[new_capacity];
        size_t index = 0;
        for (u64 bits : stored_bitset) {
            for (size_t bit = 0; bits; ++bit, bits >>= 1) {
                const size_t i = index + bit;
                if ((bits & 1) == 0) {
                    continue;
                }
                T& old_value = values[i].object;
                new (&new_values[i].object) T(std::move(old_value));
                old_value.~T();
            }
            index += 64;
        }

        stored_bitset.resize((new_capacity + 63) / 64);

        const size_t old_free_size = free_list.size();
        free_list.resize(old_free_size + (new_capacity - values_capacity));
        std::iota(free_list.begin() + old_free_size, free_list.end(),
                  static_cast<u32>(values_capacity));

        delete[] values;
        values = new_values;
        values_capacity = new_capacity;
    }

    Entry* values = nullptr;
    size_t values_capacity = 0;

    std::vector<u64> stored_bitset;
    std::vector<u32> free_list;
};

} // namespace VideoCommon

template <>
struct std::hash<VideoCommon::SlotId> {
    size_t operator()(const VideoCommon::SlotId& id) const noexcept {
        return std::hash<u32>{}(id.index);
    }
};
