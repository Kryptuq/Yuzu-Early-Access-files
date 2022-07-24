// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>
#include <utility>
#include <vector>

#include "common/common_types.h"

namespace Common {

template <typename BaseAddr>
class MultiLevelPageTable final {
public:
    constexpr MultiLevelPageTable() = default;
    explicit MultiLevelPageTable(std::size_t address_space_bits, std::size_t first_level_bits,
                                 std::size_t page_bits);

    ~MultiLevelPageTable() noexcept;

    MultiLevelPageTable(const MultiLevelPageTable&) = delete;
    MultiLevelPageTable& operator=(const MultiLevelPageTable&) = delete;

    MultiLevelPageTable(MultiLevelPageTable&& other) noexcept
        : address_space_bits{std::exchange(other.address_space_bits, 0)},
          first_level_bits{std::exchange(other.first_level_bits, 0)}, page_bits{std::exchange(
                                                                          other.page_bits, 0)},
          first_level_shift{std::exchange(other.first_level_shift, 0)},
          first_level_chunk_size{std::exchange(other.first_level_chunk_size, 0)},
          first_level_map{std::move(other.first_level_map)}, base_ptr{std::exchange(other.base_ptr,
                                                                                    nullptr)} {}

    MultiLevelPageTable& operator=(MultiLevelPageTable&& other) noexcept {
        address_space_bits = std::exchange(other.address_space_bits, 0);
        first_level_bits = std::exchange(other.first_level_bits, 0);
        page_bits = std::exchange(other.page_bits, 0);
        first_level_shift = std::exchange(other.first_level_shift, 0);
        first_level_chunk_size = std::exchange(other.first_level_chunk_size, 0);
        alloc_size = std::exchange(other.alloc_size, 0);
        first_level_map = std::move(other.first_level_map);
        base_ptr = std::exchange(other.base_ptr, nullptr);
        return *this;
    }

    void ReserveRange(u64 start, std::size_t size);

    [[nodiscard]] constexpr const BaseAddr& operator[](std::size_t index) const {
        return base_ptr[index];
    }

    [[nodiscard]] constexpr BaseAddr& operator[](std::size_t index) {
        return base_ptr[index];
    }

    [[nodiscard]] constexpr BaseAddr* data() {
        return base_ptr;
    }

    [[nodiscard]] constexpr const BaseAddr* data() const {
        return base_ptr;
    }

private:
    void AllocateLevel(u64 level);

    std::size_t address_space_bits{};
    std::size_t first_level_bits{};
    std::size_t page_bits{};
    std::size_t first_level_shift{};
    std::size_t first_level_chunk_size{};
    std::size_t alloc_size{};
    std::vector<void*> first_level_map{};
    BaseAddr* base_ptr{};
};

} // namespace Common
