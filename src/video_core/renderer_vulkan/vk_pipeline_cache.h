// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

#include "common/common_types.h"
#include "video_core/engines/const_buffer_engine_interface.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_vulkan/fixed_pipeline_state.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_shader_decompiler.h"
#include "video_core/shader/async_shaders.h"
#include "video_core/shader/memory_util.h"
#include "video_core/shader/registry.h"
#include "video_core/shader/shader_ir.h"
#include "video_core/shader_cache.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Core {
class System;
}

namespace Vulkan {

class Device;
class RasterizerVulkan;
class VKComputePipeline;
class VKDescriptorPool;
class VKScheduler;
class VKUpdateDescriptorQueue;

using Maxwell = Tegra::Engines::Maxwell3D::Regs;

struct ComputePipelineCacheKey {
    GPUVAddr shader;
    u32 shared_memory_size;
    std::array<u32, 3> workgroup_size;

    std::size_t Hash() const noexcept;

    bool operator==(const ComputePipelineCacheKey& rhs) const noexcept;

    bool operator!=(const ComputePipelineCacheKey& rhs) const noexcept {
        return !operator==(rhs);
    }
};
static_assert(std::has_unique_object_representations_v<ComputePipelineCacheKey>);
static_assert(std::is_trivially_copyable_v<ComputePipelineCacheKey>);
static_assert(std::is_trivially_constructible_v<ComputePipelineCacheKey>);

} // namespace Vulkan

namespace std {

template <>
struct hash<Vulkan::GraphicsPipelineCacheKey> {
    std::size_t operator()(const Vulkan::GraphicsPipelineCacheKey& k) const noexcept {
        return k.Hash();
    }
};

template <>
struct hash<Vulkan::ComputePipelineCacheKey> {
    std::size_t operator()(const Vulkan::ComputePipelineCacheKey& k) const noexcept {
        return k.Hash();
    }
};

} // namespace std

namespace Vulkan {

class Shader {
public:
    explicit Shader(Tegra::Engines::ConstBufferEngineInterface& engine_,
                    Tegra::Engines::ShaderType stage_, GPUVAddr gpu_addr, VAddr cpu_addr_,
                    VideoCommon::Shader::ProgramCode program_code, u32 main_offset_);
    ~Shader();

    GPUVAddr GetGpuAddr() const {
        return gpu_addr;
    }

    VideoCommon::Shader::ShaderIR& GetIR() {
        return shader_ir;
    }

    const VideoCommon::Shader::ShaderIR& GetIR() const {
        return shader_ir;
    }

    const VideoCommon::Shader::Registry& GetRegistry() const {
        return registry;
    }

    const ShaderEntries& GetEntries() const {
        return entries;
    }

private:
    GPUVAddr gpu_addr{};
    VideoCommon::Shader::ProgramCode program_code;
    VideoCommon::Shader::Registry registry;
    VideoCommon::Shader::ShaderIR shader_ir;
    ShaderEntries entries;
};

class VKPipelineCache final : public VideoCommon::ShaderCache<Shader> {
public:
    explicit VKPipelineCache(RasterizerVulkan& rasterizer, Tegra::GPU& gpu,
                             Tegra::Engines::Maxwell3D& maxwell3d,
                             Tegra::Engines::KeplerCompute& kepler_compute,
                             Tegra::MemoryManager& gpu_memory, const Device& device,
                             VKScheduler& scheduler, VKDescriptorPool& descriptor_pool,
                             VKUpdateDescriptorQueue& update_descriptor_queue);
    ~VKPipelineCache() override;

    std::array<Shader*, Maxwell::MaxShaderProgram> GetShaders();

    VKGraphicsPipeline* GetGraphicsPipeline(const GraphicsPipelineCacheKey& key,
                                            u32 num_color_buffers,
                                            VideoCommon::Shader::AsyncShaders& async_shaders);

    VKComputePipeline& GetComputePipeline(const ComputePipelineCacheKey& key);

    void EmplacePipeline(std::unique_ptr<VKGraphicsPipeline> pipeline);

protected:
    void OnShaderRemoval(Shader* shader) final;

private:
    std::pair<SPIRVProgram, std::vector<VkDescriptorSetLayoutBinding>> DecompileShaders(
        const FixedPipelineState& fixed_state);

    Tegra::GPU& gpu;
    Tegra::Engines::Maxwell3D& maxwell3d;
    Tegra::Engines::KeplerCompute& kepler_compute;
    Tegra::MemoryManager& gpu_memory;

    const Device& device;
    VKScheduler& scheduler;
    VKDescriptorPool& descriptor_pool;
    VKUpdateDescriptorQueue& update_descriptor_queue;

    std::unique_ptr<Shader> null_shader;
    std::unique_ptr<Shader> null_kernel;

    std::array<Shader*, Maxwell::MaxShaderProgram> last_shaders{};

    GraphicsPipelineCacheKey last_graphics_key;
    VKGraphicsPipeline* last_graphics_pipeline = nullptr;

    std::mutex pipeline_cache;
    std::unordered_map<GraphicsPipelineCacheKey, std::unique_ptr<VKGraphicsPipeline>>
        graphics_cache;
    std::unordered_map<ComputePipelineCacheKey, std::unique_ptr<VKComputePipeline>> compute_cache;
};

void FillDescriptorUpdateTemplateEntries(
    const ShaderEntries& entries, u32& binding, u32& offset,
    std::vector<VkDescriptorUpdateTemplateEntryKHR>& template_entries);

} // namespace Vulkan
