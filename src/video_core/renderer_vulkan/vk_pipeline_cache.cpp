// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "common/bit_cast.h"
#include "common/cityhash.h"
#include "common/microprofile.h"
#include "core/core.h"
#include "core/memory.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/memory_manager.h"
#include "video_core/renderer_vulkan/fixed_pipeline_state.h"
#include "video_core/renderer_vulkan/maxwell_to_vk.h"
#include "video_core/renderer_vulkan/vk_compute_pipeline.h"
#include "video_core/renderer_vulkan/vk_descriptor_pool.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_update_descriptor.h"
#include "video_core/shader/compiler_settings.h"
#include "video_core/shader/memory_util.h"
#include "video_core/shader_cache.h"
#include "video_core/shader_notify.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

MICROPROFILE_DECLARE(Vulkan_PipelineCache);

using Tegra::Engines::ShaderType;
using VideoCommon::Shader::GetShaderAddress;
using VideoCommon::Shader::GetShaderCode;
using VideoCommon::Shader::KERNEL_MAIN_OFFSET;
using VideoCommon::Shader::ProgramCode;
using VideoCommon::Shader::STAGE_MAIN_OFFSET;

namespace {

constexpr VkDescriptorType UNIFORM_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
constexpr VkDescriptorType STORAGE_BUFFER = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
constexpr VkDescriptorType UNIFORM_TEXEL_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
constexpr VkDescriptorType COMBINED_IMAGE_SAMPLER = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
constexpr VkDescriptorType STORAGE_TEXEL_BUFFER = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
constexpr VkDescriptorType STORAGE_IMAGE = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

constexpr VideoCommon::Shader::CompilerSettings compiler_settings{
    .depth = VideoCommon::Shader::CompileDepth::FullDecompile,
    .disable_else_derivation = true,
};

constexpr std::size_t GetStageFromProgram(std::size_t program) {
    return program == 0 ? 0 : program - 1;
}

constexpr ShaderType GetStageFromProgram(Maxwell::ShaderProgram program) {
    return static_cast<ShaderType>(GetStageFromProgram(static_cast<std::size_t>(program)));
}

ShaderType GetShaderType(Maxwell::ShaderProgram program) {
    switch (program) {
    case Maxwell::ShaderProgram::VertexB:
        return ShaderType::Vertex;
    case Maxwell::ShaderProgram::TesselationControl:
        return ShaderType::TesselationControl;
    case Maxwell::ShaderProgram::TesselationEval:
        return ShaderType::TesselationEval;
    case Maxwell::ShaderProgram::Geometry:
        return ShaderType::Geometry;
    case Maxwell::ShaderProgram::Fragment:
        return ShaderType::Fragment;
    default:
        UNIMPLEMENTED_MSG("program={}", program);
        return ShaderType::Vertex;
    }
}

template <VkDescriptorType descriptor_type, class Container>
void AddBindings(std::vector<VkDescriptorSetLayoutBinding>& bindings, u32& binding,
                 VkShaderStageFlags stage_flags, const Container& container) {
    const u32 num_entries = static_cast<u32>(std::size(container));
    for (std::size_t i = 0; i < num_entries; ++i) {
        u32 count = 1;
        if constexpr (descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            // Combined image samplers can be arrayed.
            count = container[i].size;
        }
        bindings.push_back({
            .binding = binding++,
            .descriptorType = descriptor_type,
            .descriptorCount = count,
            .stageFlags = stage_flags,
            .pImmutableSamplers = nullptr,
        });
    }
}

u32 FillDescriptorLayout(const ShaderEntries& entries,
                         std::vector<VkDescriptorSetLayoutBinding>& bindings,
                         Maxwell::ShaderProgram program_type, u32 base_binding) {
    const ShaderType stage = GetStageFromProgram(program_type);
    const VkShaderStageFlags flags = MaxwellToVK::ShaderStage(stage);

    u32 binding = base_binding;
    AddBindings<UNIFORM_BUFFER>(bindings, binding, flags, entries.const_buffers);
    AddBindings<STORAGE_BUFFER>(bindings, binding, flags, entries.global_buffers);
    AddBindings<UNIFORM_TEXEL_BUFFER>(bindings, binding, flags, entries.uniform_texels);
    AddBindings<COMBINED_IMAGE_SAMPLER>(bindings, binding, flags, entries.samplers);
    AddBindings<STORAGE_TEXEL_BUFFER>(bindings, binding, flags, entries.storage_texels);
    AddBindings<STORAGE_IMAGE>(bindings, binding, flags, entries.images);
    return binding;
}

} // Anonymous namespace

std::size_t GraphicsPipelineCacheKey::Hash() const noexcept {
    const u64 hash = Common::CityHash64(reinterpret_cast<const char*>(this), Size());
    return static_cast<std::size_t>(hash);
}

bool GraphicsPipelineCacheKey::operator==(const GraphicsPipelineCacheKey& rhs) const noexcept {
    return std::memcmp(&rhs, this, Size()) == 0;
}

std::size_t ComputePipelineCacheKey::Hash() const noexcept {
    const u64 hash = Common::CityHash64(reinterpret_cast<const char*>(this), sizeof *this);
    return static_cast<std::size_t>(hash);
}

bool ComputePipelineCacheKey::operator==(const ComputePipelineCacheKey& rhs) const noexcept {
    return std::memcmp(&rhs, this, sizeof *this) == 0;
}

Shader::Shader(Tegra::Engines::ConstBufferEngineInterface& engine_, ShaderType stage_,
               GPUVAddr gpu_addr_, VAddr cpu_addr_, ProgramCode program_code_, u32 main_offset_)
    : gpu_addr(gpu_addr_), program_code(std::move(program_code_)), registry(stage_, engine_),
      shader_ir(program_code, main_offset_, compiler_settings, registry),
      entries(GenerateShaderEntries(shader_ir)) {}

Shader::~Shader() = default;

VKPipelineCache::VKPipelineCache(RasterizerVulkan& rasterizer_, Tegra::GPU& gpu_,
                                 Tegra::Engines::Maxwell3D& maxwell3d_,
                                 Tegra::Engines::KeplerCompute& kepler_compute_,
                                 Tegra::MemoryManager& gpu_memory_, const Device& device_,
                                 VKScheduler& scheduler_, VKDescriptorPool& descriptor_pool_,
                                 VKUpdateDescriptorQueue& update_descriptor_queue_)
    : VideoCommon::ShaderCache<Shader>{rasterizer_}, gpu{gpu_}, maxwell3d{maxwell3d_},
      kepler_compute{kepler_compute_}, gpu_memory{gpu_memory_}, device{device_},
      scheduler{scheduler_}, descriptor_pool{descriptor_pool_}, update_descriptor_queue{
                                                                    update_descriptor_queue_} {}

VKPipelineCache::~VKPipelineCache() = default;

std::array<Shader*, Maxwell::MaxShaderProgram> VKPipelineCache::GetShaders() {
    std::array<Shader*, Maxwell::MaxShaderProgram> shaders{};

    for (std::size_t index = 0; index < Maxwell::MaxShaderProgram; ++index) {
        const auto program{static_cast<Maxwell::ShaderProgram>(index)};

        // Skip stages that are not enabled
        if (!maxwell3d.regs.IsShaderConfigEnabled(index)) {
            continue;
        }

        const GPUVAddr gpu_addr{GetShaderAddress(maxwell3d, program)};
        const std::optional<VAddr> cpu_addr = gpu_memory.GpuToCpuAddress(gpu_addr);
        ASSERT(cpu_addr);

        Shader* result = cpu_addr ? TryGet(*cpu_addr) : null_shader.get();
        if (!result) {
            const u8* const host_ptr{gpu_memory.GetPointer(gpu_addr)};

            // No shader found - create a new one
            static constexpr u32 stage_offset = STAGE_MAIN_OFFSET;
            const auto stage = static_cast<ShaderType>(index == 0 ? 0 : index - 1);
            ProgramCode code = GetShaderCode(gpu_memory, gpu_addr, host_ptr, false);
            const std::size_t size_in_bytes = code.size() * sizeof(u64);

            auto shader = std::make_unique<Shader>(maxwell3d, stage, gpu_addr, *cpu_addr,
                                                   std::move(code), stage_offset);
            result = shader.get();

            if (cpu_addr) {
                Register(std::move(shader), *cpu_addr, size_in_bytes);
            } else {
                null_shader = std::move(shader);
            }
        }
        shaders[index] = result;
    }
    return last_shaders = shaders;
}

VKGraphicsPipeline* VKPipelineCache::GetGraphicsPipeline(
    const GraphicsPipelineCacheKey& key, u32 num_color_buffers,
    VideoCommon::Shader::AsyncShaders& async_shaders) {
    MICROPROFILE_SCOPE(Vulkan_PipelineCache);

    if (last_graphics_pipeline && last_graphics_key == key) {
        return last_graphics_pipeline;
    }
    last_graphics_key = key;

    if (device.UseAsynchronousShaders() && async_shaders.IsShaderAsync(gpu)) {
        std::unique_lock lock{pipeline_cache};
        const auto [pair, is_cache_miss] = graphics_cache.try_emplace(key);
        if (is_cache_miss) {
            gpu.ShaderNotify().MarkSharderBuilding();
            LOG_INFO(Render_Vulkan, "Compile 0x{:016X}", key.Hash());
            const auto [program, bindings] = DecompileShaders(key.fixed_state);
            async_shaders.QueueVulkanShader(this, device, scheduler, descriptor_pool,
                                            update_descriptor_queue, bindings, program, key,
                                            num_color_buffers);
        }
        last_graphics_pipeline = pair->second.get();
        return last_graphics_pipeline;
    }

    const auto [pair, is_cache_miss] = graphics_cache.try_emplace(key);
    auto& entry = pair->second;
    if (is_cache_miss) {
        gpu.ShaderNotify().MarkSharderBuilding();
        LOG_INFO(Render_Vulkan, "Compile 0x{:016X}", key.Hash());
        const auto [program, bindings] = DecompileShaders(key.fixed_state);
        entry = std::make_unique<VKGraphicsPipeline>(device, scheduler, descriptor_pool,
                                                     update_descriptor_queue, key, bindings,
                                                     program, num_color_buffers);
        gpu.ShaderNotify().MarkShaderComplete();
    }
    last_graphics_pipeline = entry.get();
    return last_graphics_pipeline;
}

VKComputePipeline& VKPipelineCache::GetComputePipeline(const ComputePipelineCacheKey& key) {
    MICROPROFILE_SCOPE(Vulkan_PipelineCache);

    const auto [pair, is_cache_miss] = compute_cache.try_emplace(key);
    auto& entry = pair->second;
    if (!is_cache_miss) {
        return *entry;
    }
    LOG_INFO(Render_Vulkan, "Compile 0x{:016X}", key.Hash());

    const GPUVAddr gpu_addr = key.shader;

    const std::optional<VAddr> cpu_addr = gpu_memory.GpuToCpuAddress(gpu_addr);
    ASSERT(cpu_addr);

    Shader* shader = cpu_addr ? TryGet(*cpu_addr) : null_kernel.get();
    if (!shader) {
        // No shader found - create a new one
        const auto host_ptr = gpu_memory.GetPointer(gpu_addr);

        ProgramCode code = GetShaderCode(gpu_memory, gpu_addr, host_ptr, true);
        const std::size_t size_in_bytes = code.size() * sizeof(u64);

        auto shader_info = std::make_unique<Shader>(kepler_compute, ShaderType::Compute, gpu_addr,
                                                    *cpu_addr, std::move(code), KERNEL_MAIN_OFFSET);
        shader = shader_info.get();

        if (cpu_addr) {
            Register(std::move(shader_info), *cpu_addr, size_in_bytes);
        } else {
            null_kernel = std::move(shader_info);
        }
    }

    const Specialization specialization{
        .base_binding = 0,
        .workgroup_size = key.workgroup_size,
        .shared_memory_size = key.shared_memory_size,
        .point_size = std::nullopt,
        .enabled_attributes = {},
        .attribute_types = {},
        .ndc_minus_one_to_one = false,
    };
    const SPIRVShader spirv_shader{Decompile(device, shader->GetIR(), ShaderType::Compute,
                                             shader->GetRegistry(), specialization),
                                   shader->GetEntries()};
    entry = std::make_unique<VKComputePipeline>(device, scheduler, descriptor_pool,
                                                update_descriptor_queue, spirv_shader);
    return *entry;
}

void VKPipelineCache::EmplacePipeline(std::unique_ptr<VKGraphicsPipeline> pipeline) {
    gpu.ShaderNotify().MarkShaderComplete();
    std::unique_lock lock{pipeline_cache};
    graphics_cache.at(pipeline->GetCacheKey()) = std::move(pipeline);
}

void VKPipelineCache::OnShaderRemoval(Shader* shader) {
    bool finished = false;
    const auto Finish = [&] {
        // TODO(Rodrigo): Instead of finishing here, wait for the fences that use this pipeline and
        // flush.
        if (finished) {
            return;
        }
        finished = true;
        scheduler.Finish();
    };

    const GPUVAddr invalidated_addr = shader->GetGpuAddr();
    for (auto it = graphics_cache.begin(); it != graphics_cache.end();) {
        auto& entry = it->first;
        if (std::find(entry.shaders.begin(), entry.shaders.end(), invalidated_addr) ==
            entry.shaders.end()) {
            ++it;
            continue;
        }
        Finish();
        it = graphics_cache.erase(it);
    }
    for (auto it = compute_cache.begin(); it != compute_cache.end();) {
        auto& entry = it->first;
        if (entry.shader != invalidated_addr) {
            ++it;
            continue;
        }
        Finish();
        it = compute_cache.erase(it);
    }
}

std::pair<SPIRVProgram, std::vector<VkDescriptorSetLayoutBinding>>
VKPipelineCache::DecompileShaders(const FixedPipelineState& fixed_state) {
    Specialization specialization;
    if (fixed_state.topology == Maxwell::PrimitiveTopology::Points) {
        float point_size;
        std::memcpy(&point_size, &fixed_state.point_size, sizeof(float));
        specialization.point_size = point_size;
        ASSERT(point_size != 0.0f);
    }
    for (std::size_t i = 0; i < Maxwell::NumVertexAttributes; ++i) {
        const auto& attribute = fixed_state.attributes[i];
        specialization.enabled_attributes[i] = attribute.enabled.Value() != 0;
        specialization.attribute_types[i] = attribute.Type();
    }
    specialization.ndc_minus_one_to_one = fixed_state.ndc_minus_one_to_one;
    specialization.early_fragment_tests = fixed_state.early_z;

    // Alpha test
    specialization.alpha_test_func =
        FixedPipelineState::UnpackComparisonOp(fixed_state.alpha_test_func.Value());
    specialization.alpha_test_ref = Common::BitCast<float>(fixed_state.alpha_test_ref);

    SPIRVProgram program;
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (std::size_t index = 1; index < Maxwell::MaxShaderProgram; ++index) {
        const auto program_enum = static_cast<Maxwell::ShaderProgram>(index);
        // Skip stages that are not enabled
        if (!maxwell3d.regs.IsShaderConfigEnabled(index)) {
            continue;
        }
        const GPUVAddr gpu_addr = GetShaderAddress(maxwell3d, program_enum);
        const std::optional<VAddr> cpu_addr = gpu_memory.GpuToCpuAddress(gpu_addr);
        Shader* const shader = cpu_addr ? TryGet(*cpu_addr) : null_shader.get();

        const std::size_t stage = index == 0 ? 0 : index - 1; // Stage indices are 0 - 5
        const ShaderType program_type = GetShaderType(program_enum);
        const auto& entries = shader->GetEntries();
        program[stage] = {
            Decompile(device, shader->GetIR(), program_type, shader->GetRegistry(), specialization),
            entries,
        };

        const u32 old_binding = specialization.base_binding;
        specialization.base_binding =
            FillDescriptorLayout(entries, bindings, program_enum, specialization.base_binding);
        ASSERT(old_binding + entries.NumBindings() == specialization.base_binding);
    }
    return {std::move(program), std::move(bindings)};
}

template <VkDescriptorType descriptor_type, class Container>
void AddEntry(std::vector<VkDescriptorUpdateTemplateEntry>& template_entries, u32& binding,
              u32& offset, const Container& container) {
    static constexpr u32 entry_size = static_cast<u32>(sizeof(DescriptorUpdateEntry));
    const u32 count = static_cast<u32>(std::size(container));

    if constexpr (descriptor_type == COMBINED_IMAGE_SAMPLER) {
        for (u32 i = 0; i < count; ++i) {
            const u32 num_samplers = container[i].size;
            template_entries.push_back({
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = num_samplers,
                .descriptorType = descriptor_type,
                .offset = offset,
                .stride = entry_size,
            });

            ++binding;
            offset += num_samplers * entry_size;
        }
        return;
    }

    if constexpr (descriptor_type == UNIFORM_TEXEL_BUFFER ||
                  descriptor_type == STORAGE_TEXEL_BUFFER) {
        // Nvidia has a bug where updating multiple texels at once causes the driver to crash.
        // Note: Fixed in driver Windows 443.24, Linux 440.66.15
        for (u32 i = 0; i < count; ++i) {
            template_entries.push_back({
                .dstBinding = binding + i,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = descriptor_type,
                .offset = static_cast<std::size_t>(offset + i * entry_size),
                .stride = entry_size,
            });
        }
    } else if (count > 0) {
        template_entries.push_back({
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = count,
            .descriptorType = descriptor_type,
            .offset = offset,
            .stride = entry_size,
        });
    }
    offset += count * entry_size;
    binding += count;
}

void FillDescriptorUpdateTemplateEntries(
    const ShaderEntries& entries, u32& binding, u32& offset,
    std::vector<VkDescriptorUpdateTemplateEntryKHR>& template_entries) {
    AddEntry<UNIFORM_BUFFER>(template_entries, offset, binding, entries.const_buffers);
    AddEntry<STORAGE_BUFFER>(template_entries, offset, binding, entries.global_buffers);
    AddEntry<UNIFORM_TEXEL_BUFFER>(template_entries, offset, binding, entries.uniform_texels);
    AddEntry<COMBINED_IMAGE_SAMPLER>(template_entries, offset, binding, entries.samplers);
    AddEntry<STORAGE_TEXEL_BUFFER>(template_entries, offset, binding, entries.storage_texels);
    AddEntry<STORAGE_IMAGE>(template_entries, offset, binding, entries.images);
}

} // namespace Vulkan
