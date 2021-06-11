// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <set>
#include <vector>

#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_type.h"
#include "video_core/shader/registry.h"
#include "video_core/shader/shader_ir.h"

namespace Vulkan {

class Device;

using Maxwell = Tegra::Engines::Maxwell3D::Regs;
using UniformTexelEntry = VideoCommon::Shader::SamplerEntry;
using SamplerEntry = VideoCommon::Shader::SamplerEntry;
using StorageTexelEntry = VideoCommon::Shader::ImageEntry;
using ImageEntry = VideoCommon::Shader::ImageEntry;

constexpr u32 DESCRIPTOR_SET = 0;

class ConstBufferEntry : public VideoCommon::Shader::ConstBuffer {
public:
    explicit constexpr ConstBufferEntry(const ConstBuffer& entry_, u32 index_)
        : ConstBuffer{entry_}, index{index_} {}

    constexpr u32 GetIndex() const {
        return index;
    }

private:
    u32 index{};
};

struct GlobalBufferEntry {
    u32 cbuf_index{};
    u32 cbuf_offset{};
    bool is_written{};
};

struct ShaderEntries {
    u32 NumBindings() const {
        return static_cast<u32>(const_buffers.size() + global_buffers.size() +
                                uniform_texels.size() + samplers.size() + storage_texels.size() +
                                images.size());
    }

    std::vector<ConstBufferEntry> const_buffers;
    std::vector<GlobalBufferEntry> global_buffers;
    std::vector<UniformTexelEntry> uniform_texels;
    std::vector<SamplerEntry> samplers;
    std::vector<StorageTexelEntry> storage_texels;
    std::vector<ImageEntry> images;
    std::set<u32> attributes;
    std::array<bool, Maxwell::NumClipDistances> clip_distances{};
    std::size_t shader_length{};
    u32 enabled_uniform_buffers{};
    bool uses_warps{};
};

struct Specialization final {
    u32 base_binding{};

    // Compute specific
    std::array<u32, 3> workgroup_size{};
    u32 shared_memory_size{};

    // Graphics specific
    std::optional<float> point_size;
    std::bitset<Maxwell::NumVertexAttributes> enabled_attributes;
    std::array<Maxwell::VertexAttribute::Type, Maxwell::NumVertexAttributes> attribute_types{};
    bool ndc_minus_one_to_one{};
    bool early_fragment_tests{};
    float alpha_test_ref{};
    Maxwell::ComparisonOp alpha_test_func{};
};
// Old gcc versions don't consider this trivially copyable.
// static_assert(std::is_trivially_copyable_v<Specialization>);

struct SPIRVShader {
    std::vector<u32> code;
    ShaderEntries entries;
};

ShaderEntries GenerateShaderEntries(const VideoCommon::Shader::ShaderIR& ir);

std::vector<u32> Decompile(const Device& device, const VideoCommon::Shader::ShaderIR& ir,
                           Tegra::Engines::ShaderType stage,
                           const VideoCommon::Shader::Registry& registry,
                           const Specialization& specialization);

} // namespace Vulkan
