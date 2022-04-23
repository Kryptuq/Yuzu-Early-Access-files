// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>

#include "common/common_types.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;

class PipelineStatistics {
public:
    explicit PipelineStatistics(const Device& device_);

    void Collect(VkPipeline pipeline);

    void Report() const;

private:
    struct Stats {
        u64 code_size{};
        u64 register_count{};
        u64 sgpr_count{};
        u64 vgpr_count{};
        u64 branches_count{};
        u64 basic_block_count{};
    };

    const Device& device;
    mutable std::mutex mutex;
    std::vector<Stats> collected_stats;
};

} // namespace Vulkan
