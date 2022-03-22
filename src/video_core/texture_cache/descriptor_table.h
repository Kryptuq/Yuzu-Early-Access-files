// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <vector>

#include "common/common_types.h"
#include "common/div_ceil.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"

namespace VideoCommon {

template <typename Descriptor>
class DescriptorTable {
public:
    explicit DescriptorTable(Tegra::MemoryManager& gpu_memory_) : gpu_memory{gpu_memory_} {}

    [[nodiscard]] bool Synchornize(GPUVAddr gpu_addr, u32 limit) {
        [[likely]] if (current_gpu_addr == gpu_addr && current_limit == limit) {
            return false;
        }
        Refresh(gpu_addr, limit);
        return true;
    }

    void Invalidate() noexcept {
        std::ranges::fill(read_descriptors, 0);
    }

    [[nodiscard]] std::pair<Descriptor, bool> Read(u32 index) {
        DEBUG_ASSERT(index <= current_limit);
        const GPUVAddr gpu_addr = current_gpu_addr + index * sizeof(Descriptor);
        std::pair<Descriptor, bool> result;
        gpu_memory.ReadBlockUnsafe(gpu_addr, &result.first, sizeof(Descriptor));
        if (IsDescriptorRead(index)) {
            result.second = result.first != descriptors[index];
        } else {
            MarkDescriptorAsRead(index);
            result.second = true;
        }
        if (result.second) {
            descriptors[index] = result.first;
        }
        return result;
    }

    [[nodiscard]] u32 Limit() const noexcept {
        return current_limit;
    }

private:
    void Refresh(GPUVAddr gpu_addr, u32 limit) {
        current_gpu_addr = gpu_addr;
        current_limit = limit;

        const size_t num_descriptors = static_cast<size_t>(limit) + 1;
        read_descriptors.clear();
        read_descriptors.resize(Common::DivCeil(num_descriptors, 64U), 0);
        descriptors.resize(num_descriptors);
    }

    void MarkDescriptorAsRead(u32 index) noexcept {
        read_descriptors[index / 64] |= 1ULL << (index % 64);
    }

    [[nodiscard]] bool IsDescriptorRead(u32 index) const noexcept {
        return (read_descriptors[index / 64] & (1ULL << (index % 64))) != 0;
    }

    Tegra::MemoryManager& gpu_memory;
    GPUVAddr current_gpu_addr{};
    u32 current_limit{};
    std::vector<u64> read_descriptors;
    std::vector<Descriptor> descriptors;
};

} // namespace VideoCommon
