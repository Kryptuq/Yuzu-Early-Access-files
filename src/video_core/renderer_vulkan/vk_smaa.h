// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;
class Scheduler;
class StagingBufferPool;

class SMAA {
public:
    explicit SMAA(const Device& device, MemoryAllocator& allocator, size_t image_count,
                  VkExtent2D extent);
    VkImageView Draw(Scheduler& scheduler, size_t image_index, VkImage source_image,
                     VkImageView source_image_view);

private:
    enum SMAAStage {
        EdgeDetection = 0,
        BlendingWeightCalculation = 1,
        NeighborhoodBlending = 2,
        MaxSMAAStage = 3,
    };

    enum StaticImageType {
        Area = 0,
        Search = 1,
        MaxStaticImage = 2,
    };

    enum DynamicImageType {
        Blend = 0,
        Edges = 1,
        Output = 2,
        MaxDynamicImage = 3,
    };

    void CreateImages();
    void CreateRenderPasses();
    void CreateSampler();
    void CreateShaders();
    void CreateDescriptorPool();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
    void CreatePipelineLayouts();
    void CreatePipelines();
    void UpdateDescriptorSets(VkImageView image_view, size_t image_index);
    void UploadImages(Scheduler& scheduler);

    const Device& m_device;
    MemoryAllocator& m_allocator;
    const VkExtent2D m_extent;
    const u32 m_image_count;

    vk::DescriptorPool m_descriptor_pool{};
    std::array<vk::DescriptorSetLayout, MaxSMAAStage> m_descriptor_set_layouts{};
    std::array<vk::PipelineLayout, MaxSMAAStage> m_pipeline_layouts{};
    std::array<vk::ShaderModule, MaxSMAAStage> m_vertex_shaders{};
    std::array<vk::ShaderModule, MaxSMAAStage> m_fragment_shaders{};
    std::array<vk::Pipeline, MaxSMAAStage> m_pipelines{};
    std::array<vk::RenderPass, MaxSMAAStage> m_renderpasses{};

    std::array<MemoryCommit, MaxStaticImage> m_static_buffer_commits;
    std::array<vk::Image, MaxStaticImage> m_static_images{};
    std::array<vk::ImageView, MaxStaticImage> m_static_image_views{};

    struct Images {
        vk::DescriptorSets descriptor_sets{};
        std::array<MemoryCommit, MaxDynamicImage> buffer_commits;
        std::array<vk::Image, MaxDynamicImage> images{};
        std::array<vk::ImageView, MaxDynamicImage> image_views{};
        std::array<vk::Framebuffer, MaxSMAAStage> framebuffers{};
    };
    std::vector<Images> m_dynamic_images{};
    bool m_images_ready{};

    vk::Sampler m_sampler{};
};

} // namespace Vulkan
