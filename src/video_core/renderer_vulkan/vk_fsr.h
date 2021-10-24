// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/math_util.h"
#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;
class VKScheduler;

class FSR {
public:
    explicit FSR(const Device& device, MemoryAllocator& memory_allocator, size_t image_count,
                 VkExtent2D output_size);
    VkImageView Draw(VKScheduler& scheduler, size_t image_index, VkImageView image_view,
                     VkExtent2D input_image_extent, const Common::Rectangle<int>& crop_rect);

private:
    void CreateDescriptorPool();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateImages();
    void CreateSampler();
    void CreateShaders();
    void CreatePipeline();
    void CreatePipelineLayout();

    void UpdateDescriptorSet(std::size_t image_index, VkImageView image_view) const;

    const Device& device;
    MemoryAllocator& memory_allocator;
    size_t image_count;
    VkExtent2D output_size;

    vk::DescriptorPool descriptor_pool;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::DescriptorSets descriptor_sets;
    vk::PipelineLayout pipeline_layout;
    vk::ShaderModule easu_shader;
    vk::ShaderModule rcas_shader;
    vk::Pipeline easu_pipeline;
    vk::Pipeline rcas_pipeline;
    vk::Sampler sampler;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> image_views;
    std::vector<MemoryCommit> buffer_commits;
};

} // namespace Vulkan
