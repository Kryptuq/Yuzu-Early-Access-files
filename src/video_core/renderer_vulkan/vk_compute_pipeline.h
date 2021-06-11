// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "video_core/renderer_vulkan/vk_descriptor_pool.h"
#include "video_core/renderer_vulkan/vk_shader_decompiler.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;
class VKScheduler;
class VKUpdateDescriptorQueue;

class VKComputePipeline final {
public:
    explicit VKComputePipeline(const Device& device_, VKScheduler& scheduler_,
                               VKDescriptorPool& descriptor_pool_,
                               VKUpdateDescriptorQueue& update_descriptor_queue_,
                               const SPIRVShader& shader_);
    ~VKComputePipeline();

    VkDescriptorSet CommitDescriptorSet();

    VkPipeline GetHandle() const {
        return *pipeline;
    }

    VkPipelineLayout GetLayout() const {
        return *layout;
    }

    const ShaderEntries& GetEntries() const {
        return entries;
    }

private:
    vk::DescriptorSetLayout CreateDescriptorSetLayout() const;

    vk::PipelineLayout CreatePipelineLayout() const;

    vk::DescriptorUpdateTemplateKHR CreateDescriptorUpdateTemplate() const;

    vk::ShaderModule CreateShaderModule(const std::vector<u32>& code) const;

    vk::Pipeline CreatePipeline() const;

    const Device& device;
    VKScheduler& scheduler;
    ShaderEntries entries;

    vk::DescriptorSetLayout descriptor_set_layout;
    DescriptorAllocator descriptor_allocator;
    VKUpdateDescriptorQueue& update_descriptor_queue;
    vk::PipelineLayout layout;
    vk::DescriptorUpdateTemplateKHR descriptor_template;
    vk::ShaderModule shader_module;
    vk::Pipeline pipeline;
};

} // namespace Vulkan
