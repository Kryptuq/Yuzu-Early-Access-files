// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>

#include "video_core/renderer_vulkan/vk_resource_pool.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;
class VKDescriptorPool;
class VKScheduler;

class DescriptorAllocator final : public ResourcePool {
public:
    explicit DescriptorAllocator(VKDescriptorPool& descriptor_pool, VkDescriptorSetLayout layout);
    ~DescriptorAllocator() override;

    DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;
    DescriptorAllocator(const DescriptorAllocator&) = delete;

    VkDescriptorSet Commit();

protected:
    void Allocate(std::size_t begin, std::size_t end) override;

private:
    VKDescriptorPool& descriptor_pool;
    const VkDescriptorSetLayout layout;

    std::vector<vk::DescriptorSets> descriptors_allocations;
};

class VKDescriptorPool final {
    friend DescriptorAllocator;

public:
    explicit VKDescriptorPool(const Device& device, VKScheduler& scheduler);
    ~VKDescriptorPool();

    VKDescriptorPool(const VKDescriptorPool&) = delete;
    VKDescriptorPool& operator=(const VKDescriptorPool&) = delete;

private:
    vk::DescriptorPool* AllocateNewPool();

    vk::DescriptorSets AllocateDescriptors(VkDescriptorSetLayout layout, std::size_t count);

    const Device& device;
    MasterSemaphore& master_semaphore;

    std::vector<vk::DescriptorPool> pools;
    vk::DescriptorPool* active_pool;
};

} // namespace Vulkan