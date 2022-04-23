// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <vector>

#include "video_core/renderer_vulkan/vk_resource_pool.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;
class MasterSemaphore;

class CommandPool final : public ResourcePool {
public:
    explicit CommandPool(MasterSemaphore& master_semaphore_, const Device& device_);
    ~CommandPool() override;

    void Allocate(size_t begin, size_t end) override;

    VkCommandBuffer Commit();

private:
    struct Pool;

    const Device& device;
    std::vector<Pool> pools;
};

} // namespace Vulkan
