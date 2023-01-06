// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>
#include <vector>

#include "common/common_types.h"

namespace VideoCommon {

class InvalidationAccumulator {
public:
    InvalidationAccumulator() = default;
    ~InvalidationAccumulator() = default;

    void Add(GPUVAddr address, size_t size) {
        const auto reset_values = [&]() {
            if (has_collected) {
                buffer.emplace_back(start_address, accumulated_size);
            }
            start_address = address;
            accumulated_size = size;
            last_collection = start_address + size;
        };
        if (address >= start_address && address + size <= last_collection) [[likely]] {
            return;
        }
        size = ((address + size + atomicity_size_mask) & atomicity_mask) - address;
        address = address & atomicity_mask;
        if (!has_collected) [[unlikely]] {
            reset_values();
            has_collected = true;
            return;
        }
        if (address != last_collection) [[unlikely]] {
            reset_values();
            return;
        }
        accumulated_size += size;
        last_collection += size;
    }

    void Clear() {
        buffer.clear();
        start_address = 0;
        last_collection = 0;
        has_collected = false;
    }

    bool AnyAccumulated() const {
        return has_collected;
    }

    template <typename Func>
    void Callback(Func&& func) {
        if (!has_collected) {
            return;
        }
        buffer.emplace_back(start_address, accumulated_size);
        for (auto& [address, size] : buffer) {
            func(address, size);
        }
    }

private:
    static constexpr size_t atomicity_bits = 5;
    static constexpr size_t atomicity_size = 1ULL << atomicity_bits;
    static constexpr size_t atomicity_size_mask = atomicity_size - 1;
    static constexpr size_t atomicity_mask = ~atomicity_size_mask;
    GPUVAddr start_address{};
    GPUVAddr last_collection{};
    size_t accumulated_size{};
    bool has_collected{};
    std::vector<std::pair<VAddr, size_t>> buffer;
};

} // namespace VideoCommon
