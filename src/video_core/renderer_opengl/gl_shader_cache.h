// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#include <bitset>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glad/glad.h>

#include "common/common_types.h"
#include "video_core/engines/shader_type.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_shader_decompiler.h"
#include "video_core/renderer_opengl/gl_shader_disk_cache.h"
#include "video_core/shader/registry.h"
#include "video_core/shader/shader_ir.h"
#include "video_core/shader_cache.h"

namespace Tegra {
class MemoryManager;
}

namespace Core::Frontend {
class EmuWindow;
}

namespace VideoCommon::Shader {
class AsyncShaders;
}

namespace OpenGL {

class Device;
class RasterizerOpenGL;

using Maxwell = Tegra::Engines::Maxwell3D::Regs;

struct ProgramHandle {
    OGLProgram source_program;
    OGLAssemblyProgram assembly_program;
};
using ProgramSharedPtr = std::shared_ptr<ProgramHandle>;

struct PrecompiledShader {
    ProgramSharedPtr program;
    std::shared_ptr<VideoCommon::Shader::Registry> registry;
    ShaderEntries entries;
};

struct ShaderParameters {
    Tegra::GPU& gpu;
    Tegra::Engines::ConstBufferEngineInterface& engine;
    ShaderDiskCacheOpenGL& disk_cache;
    const Device& device;
    VAddr cpu_addr;
    const u8* host_ptr;
    u64 unique_identifier;
};

ProgramSharedPtr BuildShader(const Device& device, Tegra::Engines::ShaderType shader_type,
                             u64 unique_identifier, const VideoCommon::Shader::ShaderIR& ir,
                             const VideoCommon::Shader::Registry& registry,
                             bool hint_retrievable = false);

class Shader final {
public:
    ~Shader();

    /// Gets the GL program handle for the shader
    GLuint GetHandle() const;

    bool IsBuilt() const;

    /// Gets the shader entries for the shader
    const ShaderEntries& GetEntries() const {
        return entries;
    }

    const VideoCommon::Shader::Registry& GetRegistry() const {
        return *registry;
    }

    /// Mark a OpenGL shader as built
    void AsyncOpenGLBuilt(OGLProgram new_program);

    /// Mark a GLASM shader as built
    void AsyncGLASMBuilt(OGLAssemblyProgram new_program);

    static std::unique_ptr<Shader> CreateStageFromMemory(
        const ShaderParameters& params, Maxwell::ShaderProgram program_type,
        ProgramCode program_code, ProgramCode program_code_b,
        VideoCommon::Shader::AsyncShaders& async_shaders, VAddr cpu_addr);

    static std::unique_ptr<Shader> CreateKernelFromMemory(const ShaderParameters& params,
                                                          ProgramCode code);

    static std::unique_ptr<Shader> CreateFromCache(const ShaderParameters& params,
                                                   const PrecompiledShader& precompiled_shader);

private:
    explicit Shader(std::shared_ptr<VideoCommon::Shader::Registry> registry, ShaderEntries entries,
                    ProgramSharedPtr program, bool is_built_ = true);

    std::shared_ptr<VideoCommon::Shader::Registry> registry;
    ShaderEntries entries;
    ProgramSharedPtr program;
    GLuint handle = 0;
    bool is_built{};
};

class ShaderCacheOpenGL final : public VideoCommon::ShaderCache<Shader> {
public:
    explicit ShaderCacheOpenGL(RasterizerOpenGL& rasterizer_,
                               Core::Frontend::EmuWindow& emu_window_, Tegra::GPU& gpu,
                               Tegra::Engines::Maxwell3D& maxwell3d_,
                               Tegra::Engines::KeplerCompute& kepler_compute_,
                               Tegra::MemoryManager& gpu_memory_, const Device& device_);
    ~ShaderCacheOpenGL() override;

    /// Loads disk cache for the current game
    void LoadDiskCache(u64 title_id, std::stop_token stop_loading,
                       const VideoCore::DiskResourceLoadCallback& callback);

    /// Gets the current specified shader stage program
    Shader* GetStageProgram(Maxwell::ShaderProgram program,
                            VideoCommon::Shader::AsyncShaders& async_shaders);

    /// Gets a compute kernel in the passed address
    Shader* GetComputeKernel(GPUVAddr code_addr);

private:
    ProgramSharedPtr GeneratePrecompiledProgram(
        const ShaderDiskCacheEntry& entry, const ShaderDiskCachePrecompiled& precompiled_entry,
        const std::unordered_set<GLenum>& supported_formats);

    Core::Frontend::EmuWindow& emu_window;
    Tegra::GPU& gpu;
    Tegra::MemoryManager& gpu_memory;
    Tegra::Engines::Maxwell3D& maxwell3d;
    Tegra::Engines::KeplerCompute& kepler_compute;
    const Device& device;

    ShaderDiskCacheOpenGL disk_cache;
    std::unordered_map<u64, PrecompiledShader> runtime_cache;

    std::unique_ptr<Shader> null_shader;
    std::unique_ptr<Shader> null_kernel;

    std::array<Shader*, Maxwell::MaxShaderProgram> last_shaders{};
};

} // namespace OpenGL
