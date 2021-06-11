// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "common/common_types.h"
#include "common/hash.h"
#include "video_core/engines/const_buffer_engine_interface.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_type.h"
#include "video_core/guest_driver.h"

namespace VideoCommon::Shader {

struct SeparateSamplerKey {
    std::pair<u32, u32> buffers;
    std::pair<u32, u32> offsets;
};

} // namespace VideoCommon::Shader

namespace std {

template <>
struct hash<VideoCommon::Shader::SeparateSamplerKey> {
    std::size_t operator()(const VideoCommon::Shader::SeparateSamplerKey& key) const noexcept {
        return std::hash<u32>{}(key.buffers.first ^ key.buffers.second ^ key.offsets.first ^
                                key.offsets.second);
    }
};

template <>
struct equal_to<VideoCommon::Shader::SeparateSamplerKey> {
    bool operator()(const VideoCommon::Shader::SeparateSamplerKey& lhs,
                    const VideoCommon::Shader::SeparateSamplerKey& rhs) const noexcept {
        return lhs.buffers == rhs.buffers && lhs.offsets == rhs.offsets;
    }
};

} // namespace std

namespace VideoCommon::Shader {

using KeyMap = std::unordered_map<std::pair<u32, u32>, u32, Common::PairHash>;
using BoundSamplerMap = std::unordered_map<u32, Tegra::Engines::SamplerDescriptor>;
using SeparateSamplerMap =
    std::unordered_map<SeparateSamplerKey, Tegra::Engines::SamplerDescriptor>;
using BindlessSamplerMap =
    std::unordered_map<std::pair<u32, u32>, Tegra::Engines::SamplerDescriptor, Common::PairHash>;

struct GraphicsInfo {
    using Maxwell = Tegra::Engines::Maxwell3D::Regs;

    std::array<Maxwell::TransformFeedbackLayout, Maxwell::NumTransformFeedbackBuffers>
        tfb_layouts{};
    std::array<std::array<u8, 128>, Maxwell::NumTransformFeedbackBuffers> tfb_varying_locs{};
    Maxwell::PrimitiveTopology primitive_topology{};
    Maxwell::TessellationPrimitive tessellation_primitive{};
    Maxwell::TessellationSpacing tessellation_spacing{};
    bool tfb_enabled = false;
    bool tessellation_clockwise = false;
};
static_assert(std::is_trivially_copyable_v<GraphicsInfo> &&
              std::is_standard_layout_v<GraphicsInfo>);

struct ComputeInfo {
    std::array<u32, 3> workgroup_size{};
    u32 shared_memory_size_in_words = 0;
    u32 local_memory_size_in_words = 0;
};
static_assert(std::is_trivially_copyable_v<ComputeInfo> && std::is_standard_layout_v<ComputeInfo>);

struct SerializedRegistryInfo {
    VideoCore::GuestDriverProfile guest_driver_profile;
    u32 bound_buffer = 0;
    GraphicsInfo graphics;
    ComputeInfo compute;
};

/**
 * The Registry is a class use to interface the 3D and compute engines with the shader compiler.
 * With it, the shader can obtain required data from GPU state and store it for disk shader
 * compilation.
 */
class Registry {
public:
    explicit Registry(Tegra::Engines::ShaderType shader_stage, const SerializedRegistryInfo& info);

    explicit Registry(Tegra::Engines::ShaderType shader_stage,
                      Tegra::Engines::ConstBufferEngineInterface& engine_);

    ~Registry();

    /// Retrieves a key from the registry, if it's registered, it will give the registered value, if
    /// not it will obtain it from maxwell3d and register it.
    std::optional<u32> ObtainKey(u32 buffer, u32 offset);

    std::optional<Tegra::Engines::SamplerDescriptor> ObtainBoundSampler(u32 offset);

    std::optional<Tegra::Engines::SamplerDescriptor> ObtainSeparateSampler(
        std::pair<u32, u32> buffers, std::pair<u32, u32> offsets);

    std::optional<Tegra::Engines::SamplerDescriptor> ObtainBindlessSampler(u32 buffer, u32 offset);

    /// Inserts a key.
    void InsertKey(u32 buffer, u32 offset, u32 value);

    /// Inserts a bound sampler key.
    void InsertBoundSampler(u32 offset, Tegra::Engines::SamplerDescriptor sampler);

    /// Inserts a bindless sampler key.
    void InsertBindlessSampler(u32 buffer, u32 offset, Tegra::Engines::SamplerDescriptor sampler);

    /// Checks keys and samplers against engine's current const buffers.
    /// Returns true if they are the same value, false otherwise.
    bool IsConsistent() const;

    /// Returns true if the keys are equal to the other ones in the registry.
    bool HasEqualKeys(const Registry& rhs) const;

    /// Returns graphics information from this shader
    const GraphicsInfo& GetGraphicsInfo() const;

    /// Returns compute information from this shader
    const ComputeInfo& GetComputeInfo() const;

    /// Gives an getter to the const buffer keys in the database.
    const KeyMap& GetKeys() const {
        return keys;
    }

    /// Gets samplers database.
    const BoundSamplerMap& GetBoundSamplers() const {
        return bound_samplers;
    }

    /// Gets bindless samplers database.
    const BindlessSamplerMap& GetBindlessSamplers() const {
        return bindless_samplers;
    }

    /// Gets bound buffer used on this shader
    u32 GetBoundBuffer() const {
        return bound_buffer;
    }

    /// Obtains access to the guest driver's profile.
    VideoCore::GuestDriverProfile& AccessGuestDriverProfile() {
        return engine ? engine->AccessGuestDriverProfile() : stored_guest_driver_profile;
    }

private:
    const Tegra::Engines::ShaderType stage;
    VideoCore::GuestDriverProfile stored_guest_driver_profile;
    Tegra::Engines::ConstBufferEngineInterface* engine = nullptr;
    KeyMap keys;
    BoundSamplerMap bound_samplers;
    SeparateSamplerMap separate_samplers;
    BindlessSamplerMap bindless_samplers;
    u32 bound_buffer;
    GraphicsInfo graphics_info;
    ComputeInfo compute_info;
};

} // namespace VideoCommon::Shader
