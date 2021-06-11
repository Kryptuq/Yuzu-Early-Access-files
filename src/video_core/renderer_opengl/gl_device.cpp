// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include <glad/glad.h>

#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "video_core/renderer_opengl/gl_device.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"

namespace OpenGL {
namespace {
// One uniform block is reserved for emulation purposes
constexpr u32 ReservedUniformBlocks = 1;

constexpr u32 NumStages = 5;

constexpr std::array LIMIT_UBOS = {
    GL_MAX_VERTEX_UNIFORM_BLOCKS,          GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,
    GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS, GL_MAX_GEOMETRY_UNIFORM_BLOCKS,
    GL_MAX_FRAGMENT_UNIFORM_BLOCKS,        GL_MAX_COMPUTE_UNIFORM_BLOCKS,
};
constexpr std::array LIMIT_SSBOS = {
    GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,          GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,
    GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,
    GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,        GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,
};
constexpr std::array LIMIT_SAMPLERS = {
    GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
    GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,
    GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,
    GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,
    GL_MAX_TEXTURE_IMAGE_UNITS,
    GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,
};
constexpr std::array LIMIT_IMAGES = {
    GL_MAX_VERTEX_IMAGE_UNIFORMS,          GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,
    GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, GL_MAX_GEOMETRY_IMAGE_UNIFORMS,
    GL_MAX_FRAGMENT_IMAGE_UNIFORMS,        GL_MAX_COMPUTE_IMAGE_UNIFORMS,
};

template <typename T>
T GetInteger(GLenum pname) {
    GLint temporary;
    glGetIntegerv(pname, &temporary);
    return static_cast<T>(temporary);
}

bool TestProgram(const GLchar* glsl) {
    const GLuint shader{glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl)};
    GLint link_status;
    glGetProgramiv(shader, GL_LINK_STATUS, &link_status);
    glDeleteProgram(shader);
    return link_status == GL_TRUE;
}

std::vector<std::string_view> GetExtensions() {
    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    std::vector<std::string_view> extensions;
    extensions.reserve(num_extensions);
    for (GLint index = 0; index < num_extensions; ++index) {
        extensions.push_back(
            reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(index))));
    }
    return extensions;
}

bool HasExtension(std::span<const std::string_view> extensions, std::string_view extension) {
    return std::ranges::find(extensions, extension) != extensions.end();
}

u32 Extract(u32& base, u32& num, u32 amount, std::optional<GLenum> limit = {}) {
    ASSERT(num >= amount);
    if (limit) {
        amount = std::min(amount, GetInteger<u32>(*limit));
    }
    num -= amount;
    return std::exchange(base, base + amount);
}

std::array<u32, Tegra::Engines::MaxShaderTypes> BuildMaxUniformBuffers() noexcept {
    std::array<u32, Tegra::Engines::MaxShaderTypes> max;
    std::ranges::transform(LIMIT_UBOS, max.begin(),
                           [](GLenum pname) { return GetInteger<u32>(pname); });
    return max;
}

std::array<Device::BaseBindings, Tegra::Engines::MaxShaderTypes> BuildBaseBindings() noexcept {
    std::array<Device::BaseBindings, Tegra::Engines::MaxShaderTypes> bindings;

    static constexpr std::array<std::size_t, 5> stage_swizzle{0, 1, 2, 3, 4};
    const u32 total_ubos = GetInteger<u32>(GL_MAX_UNIFORM_BUFFER_BINDINGS);
    const u32 total_ssbos = GetInteger<u32>(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS);
    const u32 total_samplers = GetInteger<u32>(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);

    u32 num_ubos = total_ubos - ReservedUniformBlocks;
    u32 num_ssbos = total_ssbos;
    u32 num_samplers = total_samplers;

    u32 base_ubo = ReservedUniformBlocks;
    u32 base_ssbo = 0;
    u32 base_samplers = 0;

    for (std::size_t i = 0; i < NumStages; ++i) {
        const std::size_t stage = stage_swizzle[i];
        bindings[stage] = {
            Extract(base_ubo, num_ubos, total_ubos / NumStages, LIMIT_UBOS[stage]),
            Extract(base_ssbo, num_ssbos, total_ssbos / NumStages, LIMIT_SSBOS[stage]),
            Extract(base_samplers, num_samplers, total_samplers / NumStages,
                    LIMIT_SAMPLERS[stage])};
    }

    u32 num_images = GetInteger<u32>(GL_MAX_IMAGE_UNITS);
    u32 base_images = 0;

    // GL_MAX_IMAGE_UNITS is guaranteed by the spec to have a minimum value of 8.
    // Due to the limitation of GL_MAX_IMAGE_UNITS, reserve at least 4 image bindings on the
    // fragment stage, and at least 1 for the rest of the stages.
    // So far games are observed to use 1 image binding on vertex and 4 on fragment stages.

    // Reserve at least 4 image bindings on the fragment stage.
    bindings[4].image =
        Extract(base_images, num_images, std::max(4U, num_images / NumStages), LIMIT_IMAGES[4]);

    // This is guaranteed to be at least 1.
    const u32 total_extracted_images = num_images / (NumStages - 1);

    // Reserve the other image bindings.
    for (std::size_t i = 0; i < NumStages; ++i) {
        const std::size_t stage = stage_swizzle[i];
        if (stage == 4) {
            continue;
        }
        bindings[stage].image =
            Extract(base_images, num_images, total_extracted_images, LIMIT_IMAGES[stage]);
    }

    // Compute doesn't care about any of this.
    bindings[5] = {0, 0, 0, 0};

    return bindings;
}

bool IsASTCSupported() {
    static constexpr std::array targets = {GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY};
    static constexpr std::array formats = {
        GL_COMPRESSED_RGBA_ASTC_4x4_KHR,           GL_COMPRESSED_RGBA_ASTC_5x4_KHR,
        GL_COMPRESSED_RGBA_ASTC_5x5_KHR,           GL_COMPRESSED_RGBA_ASTC_6x5_KHR,
        GL_COMPRESSED_RGBA_ASTC_6x6_KHR,           GL_COMPRESSED_RGBA_ASTC_8x5_KHR,
        GL_COMPRESSED_RGBA_ASTC_8x6_KHR,           GL_COMPRESSED_RGBA_ASTC_8x8_KHR,
        GL_COMPRESSED_RGBA_ASTC_10x5_KHR,          GL_COMPRESSED_RGBA_ASTC_10x6_KHR,
        GL_COMPRESSED_RGBA_ASTC_10x8_KHR,          GL_COMPRESSED_RGBA_ASTC_10x10_KHR,
        GL_COMPRESSED_RGBA_ASTC_12x10_KHR,         GL_COMPRESSED_RGBA_ASTC_12x12_KHR,
        GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,   GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,
        GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,   GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,
        GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,   GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,
        GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,   GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,
        GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,  GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,
        GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,  GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,
        GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,
    };
    static constexpr std::array required_support = {
        GL_VERTEX_TEXTURE,   GL_TESS_CONTROL_TEXTURE, GL_TESS_EVALUATION_TEXTURE,
        GL_GEOMETRY_TEXTURE, GL_FRAGMENT_TEXTURE,     GL_COMPUTE_TEXTURE,
    };

    for (const GLenum target : targets) {
        for (const GLenum format : formats) {
            for (const GLenum support : required_support) {
                GLint value;
                glGetInternalformativ(target, format, support, 1, &value);
                if (value != GL_FULL_SUPPORT) {
                    return false;
                }
            }
        }
    }
    return true;
}

[[nodiscard]] bool IsDebugToolAttached(std::span<const std::string_view> extensions) {
    const bool nsight = std::getenv("NVTX_INJECTION64_PATH") || std::getenv("NSIGHT_LAUNCHED");
    return nsight || HasExtension(extensions, "GL_EXT_debug_tool");
}
} // Anonymous namespace

Device::Device() {
    if (!GLAD_GL_VERSION_4_6) {
        LOG_ERROR(Render_OpenGL, "OpenGL 4.6 is not available");
        throw std::runtime_error{"Insufficient version"};
    }
    const std::string_view vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const std::string_view version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const std::vector extensions = GetExtensions();

    const bool is_nvidia = vendor == "NVIDIA Corporation";
    const bool is_amd = vendor == "ATI Technologies Inc.";
    const bool is_intel = vendor == "Intel";

#ifdef __unix__
    const bool is_linux = true;
#else
    const bool is_linux = false;
#endif

    bool disable_fast_buffer_sub_data = false;
    if (is_nvidia && version == "4.6.0 NVIDIA 443.24") {
        LOG_WARNING(
            Render_OpenGL,
            "Beta driver 443.24 is known to have issues. There might be performance issues.");
        disable_fast_buffer_sub_data = true;
    }

    max_uniform_buffers = BuildMaxUniformBuffers();
    base_bindings = BuildBaseBindings();
    uniform_buffer_alignment = GetInteger<size_t>(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    shader_storage_alignment = GetInteger<size_t>(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
    max_vertex_attributes = GetInteger<u32>(GL_MAX_VERTEX_ATTRIBS);
    max_varyings = GetInteger<u32>(GL_MAX_VARYING_VECTORS);
    max_compute_shared_memory_size = GetInteger<u32>(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
    has_warp_intrinsics = GLAD_GL_NV_gpu_shader5 && GLAD_GL_NV_shader_thread_group &&
                          GLAD_GL_NV_shader_thread_shuffle;
    has_shader_ballot = GLAD_GL_ARB_shader_ballot;
    has_vertex_viewport_layer = GLAD_GL_ARB_shader_viewport_layer_array;
    has_image_load_formatted = HasExtension(extensions, "GL_EXT_shader_image_load_formatted");
    has_texture_shadow_lod = HasExtension(extensions, "GL_EXT_texture_shadow_lod");
    has_astc = IsASTCSupported();
    has_variable_aoffi = TestVariableAoffi();
    has_component_indexing_bug = is_amd;
    has_precise_bug = TestPreciseBug();
    has_broken_texture_view_formats = is_amd || (!is_linux && is_intel);
    has_nv_viewport_array2 = GLAD_GL_NV_viewport_array2;
    has_vertex_buffer_unified_memory = GLAD_GL_NV_vertex_buffer_unified_memory;
    has_debugging_tool_attached = IsDebugToolAttached(extensions);
    has_depth_buffer_float = HasExtension(extensions, "GL_NV_depth_buffer_float");

    // At the moment of writing this, only Nvidia's driver optimizes BufferSubData on exclusive
    // uniform buffers as "push constants"
    has_fast_buffer_sub_data = is_nvidia && !disable_fast_buffer_sub_data;

    use_assembly_shaders = Settings::values.use_assembly_shaders.GetValue() &&
                           GLAD_GL_NV_gpu_program5 && GLAD_GL_NV_compute_program5 &&
                           GLAD_GL_NV_transform_feedback && GLAD_GL_NV_transform_feedback2;

    // Blocks AMD and Intel OpenGL drivers on Windows from using asynchronous shader compilation.
    use_asynchronous_shaders = Settings::values.use_asynchronous_shaders.GetValue() &&
                               !(is_amd || (is_intel && !is_linux));
    use_driver_cache = is_nvidia;

    LOG_INFO(Render_OpenGL, "Renderer_VariableAOFFI: {}", has_variable_aoffi);
    LOG_INFO(Render_OpenGL, "Renderer_ComponentIndexingBug: {}", has_component_indexing_bug);
    LOG_INFO(Render_OpenGL, "Renderer_PreciseBug: {}", has_precise_bug);
    LOG_INFO(Render_OpenGL, "Renderer_BrokenTextureViewFormats: {}",
             has_broken_texture_view_formats);

    if (Settings::values.use_assembly_shaders.GetValue() && !use_assembly_shaders) {
        LOG_ERROR(Render_OpenGL, "Assembly shaders enabled but not supported");
    }

    if (Settings::values.use_asynchronous_shaders.GetValue() && !use_asynchronous_shaders) {
        LOG_WARNING(Render_OpenGL, "Asynchronous shader compilation enabled but not supported");
    }
}

Device::Device(std::nullptr_t) {
    max_uniform_buffers.fill(std::numeric_limits<u32>::max());
    uniform_buffer_alignment = 4;
    shader_storage_alignment = 4;
    max_vertex_attributes = 16;
    max_varyings = 15;
    max_compute_shared_memory_size = 0x10000;
    has_warp_intrinsics = true;
    has_shader_ballot = true;
    has_vertex_viewport_layer = true;
    has_image_load_formatted = true;
    has_texture_shadow_lod = true;
    has_variable_aoffi = true;
    has_depth_buffer_float = true;
}

bool Device::TestVariableAoffi() {
    return TestProgram(R"(#version 430 core
// This is a unit test, please ignore me on apitrace bug reports.
uniform sampler2D tex;
uniform ivec2 variable_offset;
out vec4 output_attribute;
void main() {
    output_attribute = textureOffset(tex, vec2(0), variable_offset);
})");
}

bool Device::TestPreciseBug() {
    return !TestProgram(R"(#version 430 core
in vec3 coords;
out float out_value;
uniform sampler2DShadow tex;
void main() {
    precise float tmp_value = vec4(texture(tex, coords)).x;
    out_value = tmp_value;
})");
}

} // namespace OpenGL
