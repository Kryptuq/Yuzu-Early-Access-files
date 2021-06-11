// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <optional>
#include <vector>

#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_vulkan/fixed_pipeline_state.h"
#include "video_core/renderer_vulkan/vk_descriptor_pool.h"
#include "video_core/renderer_vulkan/vk_shader_decompiler.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

using Maxwell = Tegra::Engines::Maxwell3D::Regs;

struct GraphicsPipelineCacheKey {
    VkRenderPass renderpass;
    std::array<GPUVAddr, Maxwell::MaxShaderProgram> shaders;
    FixedPipelineState fixed_state;

    std::size_t Hash() const noexcept;

    bool operator==(const GraphicsPipelineCacheKey& rhs) const noexcept;

    bool operator!=(const GraphicsPipelineCacheKey& rhs) const noexcept {
        return !operator==(rhs);
    }

    std::size_t Size() const noexcept {
        return sizeof(renderpass) + sizeof(shaders) + fixed_state.Size();
    }
};
static_assert(std::has_unique_object_representations_v<GraphicsPipelineCacheKey>);
static_assert(std::is_trivially_copyable_v<GraphicsPipelineCacheKey>);
static_assert(std::is_trivially_constructible_v<GraphicsPipelineCacheKey>);

class Device;
class VKDescriptorPool;
class VKScheduler;
class VKUpdateDescriptorQueue;

using SPIRVProgram = std::array<std::optional<SPIRVShader>, Maxwell::MaxShaderStage>;

class VKGraphicsPipeline final {
public:
    explicit VKGraphicsPipeline(const Device& device_, VKScheduler& scheduler_,
                                VKDescriptorPool& descriptor_pool,
                                VKUpdateDescriptorQueue& update_descriptor_queue_,
                                const GraphicsPipelineCacheKey& key,
                                vk::Span<VkDescriptorSetLayoutBinding> bindings,
                                const SPIRVProgram& program, u32 num_color_buffers);
    ~VKGraphicsPipeline();

    VkDescriptorSet CommitDescriptorSet();

    VkPipeline GetHandle() const {
        return *pipeline;
    }

    VkPipelineLayout GetLayout() const {
        return *layout;
    }

    GraphicsPipelineCacheKey GetCacheKey() const {
        return cache_key;
    }

private:
    vk::DescriptorSetLayout CreateDescriptorSetLayout(
        vk::Span<VkDescriptorSetLayoutBinding> bindings) const;

    vk::PipelineLayout CreatePipelineLayout() const;

    vk::DescriptorUpdateTemplateKHR CreateDescriptorUpdateTemplate(
        const SPIRVProgram& program) const;

    std::vector<vk::ShaderModule> CreateShaderModules(const SPIRVProgram& program) const;

    vk::Pipeline CreatePipeline(const SPIRVProgram& program, VkRenderPass renderpass,
                                u32 num_color_buffers) const;

    const Device& device;
    VKScheduler& scheduler;
    const GraphicsPipelineCacheKey cache_key;
    const u64 hash;

    vk::DescriptorSetLayout descriptor_set_layout;
    DescriptorAllocator descriptor_allocator;
    VKUpdateDescriptorQueue& update_descriptor_queue;
    vk::PipelineLayout layout;
    vk::DescriptorUpdateTemplateKHR descriptor_template;
    std::vector<vk::ShaderModule> modules;

    vk::Pipeline pipeline;
};

} // namespace Vulkan
