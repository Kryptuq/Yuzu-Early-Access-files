// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/frontend/emu_window.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_type.h"
#include "video_core/memory_manager.h"
#include "video_core/renderer_opengl/gl_arb_decompiler.h"
#include "video_core/renderer_opengl/gl_rasterizer.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_shader_cache.h"
#include "video_core/renderer_opengl/gl_shader_decompiler.h"
#include "video_core/renderer_opengl/gl_shader_disk_cache.h"
#include "video_core/renderer_opengl/gl_state_tracker.h"
#include "video_core/shader/memory_util.h"
#include "video_core/shader/registry.h"
#include "video_core/shader/shader_ir.h"
#include "video_core/shader_cache.h"
#include "video_core/shader_notify.h"

namespace OpenGL {

using Tegra::Engines::ShaderType;
using VideoCommon::Shader::GetShaderAddress;
using VideoCommon::Shader::GetShaderCode;
using VideoCommon::Shader::GetUniqueIdentifier;
using VideoCommon::Shader::KERNEL_MAIN_OFFSET;
using VideoCommon::Shader::ProgramCode;
using VideoCommon::Shader::Registry;
using VideoCommon::Shader::ShaderIR;
using VideoCommon::Shader::STAGE_MAIN_OFFSET;

namespace {

constexpr VideoCommon::Shader::CompilerSettings COMPILER_SETTINGS{};

/// Gets the shader type from a Maxwell program type
constexpr GLenum GetGLShaderType(ShaderType shader_type) {
    switch (shader_type) {
    case ShaderType::Vertex:
        return GL_VERTEX_SHADER;
    case ShaderType::Geometry:
        return GL_GEOMETRY_SHADER;
    case ShaderType::Fragment:
        return GL_FRAGMENT_SHADER;
    case ShaderType::Compute:
        return GL_COMPUTE_SHADER;
    default:
        return GL_NONE;
    }
}

constexpr const char* GetShaderTypeName(ShaderType shader_type) {
    switch (shader_type) {
    case ShaderType::Vertex:
        return "VS";
    case ShaderType::TesselationControl:
        return "HS";
    case ShaderType::TesselationEval:
        return "DS";
    case ShaderType::Geometry:
        return "GS";
    case ShaderType::Fragment:
        return "FS";
    case ShaderType::Compute:
        return "CS";
    }
    return "UNK";
}

constexpr ShaderType GetShaderType(Maxwell::ShaderProgram program_type) {
    switch (program_type) {
    case Maxwell::ShaderProgram::VertexA:
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
    }
    return {};
}

constexpr GLenum AssemblyEnum(ShaderType shader_type) {
    switch (shader_type) {
    case ShaderType::Vertex:
        return GL_VERTEX_PROGRAM_NV;
    case ShaderType::TesselationControl:
        return GL_TESS_CONTROL_PROGRAM_NV;
    case ShaderType::TesselationEval:
        return GL_TESS_EVALUATION_PROGRAM_NV;
    case ShaderType::Geometry:
        return GL_GEOMETRY_PROGRAM_NV;
    case ShaderType::Fragment:
        return GL_FRAGMENT_PROGRAM_NV;
    case ShaderType::Compute:
        return GL_COMPUTE_PROGRAM_NV;
    }
    return {};
}

std::string MakeShaderID(u64 unique_identifier, ShaderType shader_type) {
    return fmt::format("{}{:016X}", GetShaderTypeName(shader_type), unique_identifier);
}

std::shared_ptr<Registry> MakeRegistry(const ShaderDiskCacheEntry& entry) {
    const VideoCore::GuestDriverProfile guest_profile{entry.texture_handler_size};
    const VideoCommon::Shader::SerializedRegistryInfo info{guest_profile, entry.bound_buffer,
                                                           entry.graphics_info, entry.compute_info};
    auto registry = std::make_shared<Registry>(entry.type, info);
    for (const auto& [address, value] : entry.keys) {
        const auto [buffer, offset] = address;
        registry->InsertKey(buffer, offset, value);
    }
    for (const auto& [offset, sampler] : entry.bound_samplers) {
        registry->InsertBoundSampler(offset, sampler);
    }
    for (const auto& [key, sampler] : entry.bindless_samplers) {
        const auto [buffer, offset] = key;
        registry->InsertBindlessSampler(buffer, offset, sampler);
    }
    return registry;
}

std::unordered_set<GLenum> GetSupportedFormats() {
    GLint num_formats;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &num_formats);

    std::vector<GLint> formats(num_formats);
    glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats.data());

    std::unordered_set<GLenum> supported_formats;
    for (const GLint format : formats) {
        supported_formats.insert(static_cast<GLenum>(format));
    }
    return supported_formats;
}

} // Anonymous namespace

ProgramSharedPtr BuildShader(const Device& device, ShaderType shader_type, u64 unique_identifier,
                             const ShaderIR& ir, const Registry& registry, bool hint_retrievable) {
    if (device.UseDriverCache()) {
        // Ignore hint retrievable if we are using the driver cache
        hint_retrievable = false;
    }
    const std::string shader_id = MakeShaderID(unique_identifier, shader_type);
    LOG_INFO(Render_OpenGL, "{}", shader_id);

    auto program = std::make_shared<ProgramHandle>();

    if (device.UseAssemblyShaders()) {
        const std::string arb =
            DecompileAssemblyShader(device, ir, registry, shader_type, shader_id);

        GLuint& arb_prog = program->assembly_program.handle;

// Commented out functions signal OpenGL errors but are compatible with apitrace.
// Use them only to capture and replay on apitrace.
#if 0
        glGenProgramsNV(1, &arb_prog);
        glLoadProgramNV(AssemblyEnum(shader_type), arb_prog, static_cast<GLsizei>(arb.size()),
                        reinterpret_cast<const GLubyte*>(arb.data()));
#else
        glGenProgramsARB(1, &arb_prog);
        glNamedProgramStringEXT(arb_prog, AssemblyEnum(shader_type), GL_PROGRAM_FORMAT_ASCII_ARB,
                                static_cast<GLsizei>(arb.size()), arb.data());
#endif
        const auto err = reinterpret_cast<const char*>(glGetString(GL_PROGRAM_ERROR_STRING_NV));
        if (err && *err) {
            LOG_CRITICAL(Render_OpenGL, "{}", err);
            LOG_INFO(Render_OpenGL, "\n{}", arb);
        }
    } else {
        const std::string glsl = DecompileShader(device, ir, registry, shader_type, shader_id);
        OGLShader shader;
        shader.Create(glsl.c_str(), GetGLShaderType(shader_type));

        program->source_program.Create(true, hint_retrievable, shader.handle);
    }

    return program;
}

Shader::Shader(std::shared_ptr<Registry> registry_, ShaderEntries entries_,
               ProgramSharedPtr program_, bool is_built_)
    : registry{std::move(registry_)}, entries{std::move(entries_)}, program{std::move(program_)},
      is_built{is_built_} {
    handle = program->assembly_program.handle;
    if (handle == 0) {
        handle = program->source_program.handle;
    }
    if (is_built) {
        ASSERT(handle != 0);
    }
}

Shader::~Shader() = default;

GLuint Shader::GetHandle() const {
    DEBUG_ASSERT(registry->IsConsistent());
    return handle;
}

bool Shader::IsBuilt() const {
    return is_built;
}

void Shader::AsyncOpenGLBuilt(OGLProgram new_program) {
    program->source_program = std::move(new_program);
    handle = program->source_program.handle;
    is_built = true;
}

void Shader::AsyncGLASMBuilt(OGLAssemblyProgram new_program) {
    program->assembly_program = std::move(new_program);
    handle = program->assembly_program.handle;
    is_built = true;
}

std::unique_ptr<Shader> Shader::CreateStageFromMemory(
    const ShaderParameters& params, Maxwell::ShaderProgram program_type, ProgramCode code,
    ProgramCode code_b, VideoCommon::Shader::AsyncShaders& async_shaders, VAddr cpu_addr) {
    const auto shader_type = GetShaderType(program_type);

    auto& gpu = params.gpu;
    gpu.ShaderNotify().MarkSharderBuilding();

    auto registry = std::make_shared<Registry>(shader_type, gpu.Maxwell3D());
    if (!async_shaders.IsShaderAsync(gpu) || !params.device.UseAsynchronousShaders()) {
        const ShaderIR ir(code, STAGE_MAIN_OFFSET, COMPILER_SETTINGS, *registry);
        // TODO(Rodrigo): Handle VertexA shaders
        // std::optional<ShaderIR> ir_b;
        // if (!code_b.empty()) {
        //     ir_b.emplace(code_b, STAGE_MAIN_OFFSET);
        // }
        auto program =
            BuildShader(params.device, shader_type, params.unique_identifier, ir, *registry);
        ShaderDiskCacheEntry entry;
        entry.type = shader_type;
        entry.code = std::move(code);
        entry.code_b = std::move(code_b);
        entry.unique_identifier = params.unique_identifier;
        entry.bound_buffer = registry->GetBoundBuffer();
        entry.graphics_info = registry->GetGraphicsInfo();
        entry.keys = registry->GetKeys();
        entry.bound_samplers = registry->GetBoundSamplers();
        entry.bindless_samplers = registry->GetBindlessSamplers();
        params.disk_cache.SaveEntry(std::move(entry));

        gpu.ShaderNotify().MarkShaderComplete();

        return std::unique_ptr<Shader>(new Shader(std::move(registry),
                                                  MakeEntries(params.device, ir, shader_type),
                                                  std::move(program), true));
    } else {
        // Required for entries
        const ShaderIR ir(code, STAGE_MAIN_OFFSET, COMPILER_SETTINGS, *registry);
        auto entries = MakeEntries(params.device, ir, shader_type);

        async_shaders.QueueOpenGLShader(params.device, shader_type, params.unique_identifier,
                                        std::move(code), std::move(code_b), STAGE_MAIN_OFFSET,
                                        COMPILER_SETTINGS, *registry, cpu_addr);

        auto program = std::make_shared<ProgramHandle>();
        return std::unique_ptr<Shader>(
            new Shader(std::move(registry), std::move(entries), std::move(program), false));
    }
}

std::unique_ptr<Shader> Shader::CreateKernelFromMemory(const ShaderParameters& params,
                                                       ProgramCode code) {
    auto& gpu = params.gpu;
    gpu.ShaderNotify().MarkSharderBuilding();

    auto registry = std::make_shared<Registry>(ShaderType::Compute, params.engine);
    const ShaderIR ir(code, KERNEL_MAIN_OFFSET, COMPILER_SETTINGS, *registry);
    const u64 uid = params.unique_identifier;
    auto program = BuildShader(params.device, ShaderType::Compute, uid, ir, *registry);

    ShaderDiskCacheEntry entry;
    entry.type = ShaderType::Compute;
    entry.code = std::move(code);
    entry.unique_identifier = uid;
    entry.bound_buffer = registry->GetBoundBuffer();
    entry.compute_info = registry->GetComputeInfo();
    entry.keys = registry->GetKeys();
    entry.bound_samplers = registry->GetBoundSamplers();
    entry.bindless_samplers = registry->GetBindlessSamplers();
    params.disk_cache.SaveEntry(std::move(entry));

    gpu.ShaderNotify().MarkShaderComplete();

    return std::unique_ptr<Shader>(new Shader(std::move(registry),
                                              MakeEntries(params.device, ir, ShaderType::Compute),
                                              std::move(program)));
}

std::unique_ptr<Shader> Shader::CreateFromCache(const ShaderParameters& params,
                                                const PrecompiledShader& precompiled_shader) {
    return std::unique_ptr<Shader>(new Shader(
        precompiled_shader.registry, precompiled_shader.entries, precompiled_shader.program));
}

ShaderCacheOpenGL::ShaderCacheOpenGL(RasterizerOpenGL& rasterizer_,
                                     Core::Frontend::EmuWindow& emu_window_, Tegra::GPU& gpu_,
                                     Tegra::Engines::Maxwell3D& maxwell3d_,
                                     Tegra::Engines::KeplerCompute& kepler_compute_,
                                     Tegra::MemoryManager& gpu_memory_, const Device& device_)
    : ShaderCache{rasterizer_}, emu_window{emu_window_}, gpu{gpu_}, gpu_memory{gpu_memory_},
      maxwell3d{maxwell3d_}, kepler_compute{kepler_compute_}, device{device_} {}

ShaderCacheOpenGL::~ShaderCacheOpenGL() = default;

void ShaderCacheOpenGL::LoadDiskCache(u64 title_id, std::stop_token stop_loading,
                                      const VideoCore::DiskResourceLoadCallback& callback) {
    disk_cache.BindTitleID(title_id);
    const std::optional transferable = disk_cache.LoadTransferable();

    LOG_INFO(Render_OpenGL, "Total Shader Count: {}",
             transferable.has_value() ? transferable->size() : 0);

    if (!transferable) {
        return;
    }

    std::vector<ShaderDiskCachePrecompiled> gl_cache;
    if (!device.UseAssemblyShaders() && !device.UseDriverCache()) {
        // Only load precompiled cache when we are not using assembly shaders
        gl_cache = disk_cache.LoadPrecompiled();
    }
    const auto supported_formats = GetSupportedFormats();

    // Track if precompiled cache was altered during loading to know if we have to
    // serialize the virtual precompiled cache file back to the hard drive
    bool precompiled_cache_altered = false;

    // Inform the frontend about shader build initialization
    if (callback) {
        callback(VideoCore::LoadCallbackStage::Build, 0, transferable->size());
    }

    std::mutex mutex;
    std::size_t built_shaders = 0; // It doesn't have be atomic since it's used behind a mutex
    std::atomic_bool gl_cache_failed = false;

    const auto find_precompiled = [&gl_cache](u64 id) {
        return std::ranges::find(gl_cache, id, &ShaderDiskCachePrecompiled::unique_identifier);
    };

    const auto worker = [&](Core::Frontend::GraphicsContext* context, std::size_t begin,
                            std::size_t end) {
        const auto scope = context->Acquire();

        for (std::size_t i = begin; i < end; ++i) {
            if (stop_loading.stop_requested()) {
                return;
            }
            const auto& entry = (*transferable)[i];
            const u64 uid = entry.unique_identifier;
            const auto it = find_precompiled(uid);
            const auto precompiled_entry = it != gl_cache.end() ? &*it : nullptr;

            const bool is_compute = entry.type == ShaderType::Compute;
            const u32 main_offset = is_compute ? KERNEL_MAIN_OFFSET : STAGE_MAIN_OFFSET;
            auto registry = MakeRegistry(entry);
            const ShaderIR ir(entry.code, main_offset, COMPILER_SETTINGS, *registry);

            ProgramSharedPtr program;
            if (precompiled_entry) {
                // If the shader is precompiled, attempt to load it with
                program = GeneratePrecompiledProgram(entry, *precompiled_entry, supported_formats);
                if (!program) {
                    gl_cache_failed = true;
                }
            }
            if (!program) {
                // Otherwise compile it from GLSL
                program = BuildShader(device, entry.type, uid, ir, *registry, true);
            }

            PrecompiledShader shader;
            shader.program = std::move(program);
            shader.registry = std::move(registry);
            shader.entries = MakeEntries(device, ir, entry.type);

            std::scoped_lock lock{mutex};
            if (callback) {
                callback(VideoCore::LoadCallbackStage::Build, ++built_shaders,
                         transferable->size());
            }
            runtime_cache.emplace(entry.unique_identifier, std::move(shader));
        }
    };

    const std::size_t num_workers{std::max(1U, std::thread::hardware_concurrency())};
    const std::size_t bucket_size{transferable->size() / num_workers};
    std::vector<std::unique_ptr<Core::Frontend::GraphicsContext>> contexts(num_workers);
    std::vector<std::thread> threads(num_workers);
    for (std::size_t i = 0; i < num_workers; ++i) {
        const bool is_last_worker = i + 1 == num_workers;
        const std::size_t start{bucket_size * i};
        const std::size_t end{is_last_worker ? transferable->size() : start + bucket_size};

        // On some platforms the shared context has to be created from the GUI thread
        contexts[i] = emu_window.CreateSharedContext();
        threads[i] = std::thread(worker, contexts[i].get(), start, end);
    }
    for (auto& thread : threads) {
        thread.join();
    }

    if (gl_cache_failed) {
        // Invalidate the precompiled cache if a shader dumped shader was rejected
        disk_cache.InvalidatePrecompiled();
        precompiled_cache_altered = true;
        return;
    }
    if (stop_loading.stop_requested()) {
        return;
    }

    if (device.UseAssemblyShaders() || device.UseDriverCache()) {
        // Don't store precompiled binaries for assembly shaders or when using the driver cache
        return;
    }

    // TODO(Rodrigo): Do state tracking for transferable shaders and do a dummy draw
    // before precompiling them

    for (std::size_t i = 0; i < transferable->size(); ++i) {
        const u64 id = (*transferable)[i].unique_identifier;
        const auto it = find_precompiled(id);
        if (it == gl_cache.end()) {
            const GLuint program = runtime_cache.at(id).program->source_program.handle;
            disk_cache.SavePrecompiled(id, program);
            precompiled_cache_altered = true;
        }
    }

    if (precompiled_cache_altered) {
        disk_cache.SaveVirtualPrecompiledFile();
    }
}

ProgramSharedPtr ShaderCacheOpenGL::GeneratePrecompiledProgram(
    const ShaderDiskCacheEntry& entry, const ShaderDiskCachePrecompiled& precompiled_entry,
    const std::unordered_set<GLenum>& supported_formats) {
    if (!supported_formats.contains(precompiled_entry.binary_format)) {
        LOG_INFO(Render_OpenGL, "Precompiled cache entry with unsupported format, removing");
        return {};
    }

    auto program = std::make_shared<ProgramHandle>();
    GLuint& handle = program->source_program.handle;
    handle = glCreateProgram();
    glProgramParameteri(handle, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glProgramBinary(handle, precompiled_entry.binary_format, precompiled_entry.binary.data(),
                    static_cast<GLsizei>(precompiled_entry.binary.size()));

    GLint link_status;
    glGetProgramiv(handle, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        LOG_INFO(Render_OpenGL, "Precompiled cache rejected by the driver, removing");
        return {};
    }

    return program;
}

Shader* ShaderCacheOpenGL::GetStageProgram(Maxwell::ShaderProgram program,
                                           VideoCommon::Shader::AsyncShaders& async_shaders) {
    if (!maxwell3d.dirty.flags[Dirty::Shaders]) {
        auto* last_shader = last_shaders[static_cast<std::size_t>(program)];
        if (last_shader->IsBuilt()) {
            return last_shader;
        }
    }

    const GPUVAddr address{GetShaderAddress(maxwell3d, program)};

    if (device.UseAsynchronousShaders() && async_shaders.HasCompletedWork()) {
        auto completed_work = async_shaders.GetCompletedWork();
        for (auto& work : completed_work) {
            Shader* shader = TryGet(work.cpu_address);
            gpu.ShaderNotify().MarkShaderComplete();
            if (shader == nullptr) {
                continue;
            }
            using namespace VideoCommon::Shader;
            if (work.backend == AsyncShaders::Backend::OpenGL) {
                shader->AsyncOpenGLBuilt(std::move(work.program.opengl));
            } else if (work.backend == AsyncShaders::Backend::GLASM) {
                shader->AsyncGLASMBuilt(std::move(work.program.glasm));
            }

            auto& registry = shader->GetRegistry();

            ShaderDiskCacheEntry entry;
            entry.type = work.shader_type;
            entry.code = std::move(work.code);
            entry.code_b = std::move(work.code_b);
            entry.unique_identifier = work.uid;
            entry.bound_buffer = registry.GetBoundBuffer();
            entry.graphics_info = registry.GetGraphicsInfo();
            entry.keys = registry.GetKeys();
            entry.bound_samplers = registry.GetBoundSamplers();
            entry.bindless_samplers = registry.GetBindlessSamplers();
            disk_cache.SaveEntry(std::move(entry));
        }
    }

    // Look up shader in the cache based on address
    const std::optional<VAddr> cpu_addr{gpu_memory.GpuToCpuAddress(address)};
    if (Shader* const shader{cpu_addr ? TryGet(*cpu_addr) : null_shader.get()}) {
        return last_shaders[static_cast<std::size_t>(program)] = shader;
    }

    const u8* const host_ptr{gpu_memory.GetPointer(address)};

    // No shader found - create a new one
    ProgramCode code{GetShaderCode(gpu_memory, address, host_ptr, false)};
    ProgramCode code_b;
    if (program == Maxwell::ShaderProgram::VertexA) {
        const GPUVAddr address_b{GetShaderAddress(maxwell3d, Maxwell::ShaderProgram::VertexB)};
        const u8* host_ptr_b = gpu_memory.GetPointer(address_b);
        code_b = GetShaderCode(gpu_memory, address_b, host_ptr_b, false);
    }
    const std::size_t code_size = code.size() * sizeof(u64);

    const u64 unique_identifier = GetUniqueIdentifier(
        GetShaderType(program), program == Maxwell::ShaderProgram::VertexA, code, code_b);

    const ShaderParameters params{gpu,       maxwell3d, disk_cache,       device,
                                  *cpu_addr, host_ptr,  unique_identifier};

    std::unique_ptr<Shader> shader;
    const auto found = runtime_cache.find(unique_identifier);
    if (found == runtime_cache.end()) {
        shader = Shader::CreateStageFromMemory(params, program, std::move(code), std::move(code_b),
                                               async_shaders, cpu_addr.value_or(0));
    } else {
        shader = Shader::CreateFromCache(params, found->second);
    }

    Shader* const result = shader.get();
    if (cpu_addr) {
        Register(std::move(shader), *cpu_addr, code_size);
    } else {
        null_shader = std::move(shader);
    }

    return last_shaders[static_cast<std::size_t>(program)] = result;
}

Shader* ShaderCacheOpenGL::GetComputeKernel(GPUVAddr code_addr) {
    const std::optional<VAddr> cpu_addr{gpu_memory.GpuToCpuAddress(code_addr)};

    if (Shader* const kernel = cpu_addr ? TryGet(*cpu_addr) : null_kernel.get()) {
        return kernel;
    }

    // No kernel found, create a new one
    const u8* host_ptr{gpu_memory.GetPointer(code_addr)};
    ProgramCode code{GetShaderCode(gpu_memory, code_addr, host_ptr, true)};
    const std::size_t code_size{code.size() * sizeof(u64)};
    const u64 unique_identifier{GetUniqueIdentifier(ShaderType::Compute, false, code)};

    const ShaderParameters params{gpu,       kepler_compute, disk_cache,       device,
                                  *cpu_addr, host_ptr,       unique_identifier};

    std::unique_ptr<Shader> kernel;
    const auto found = runtime_cache.find(unique_identifier);
    if (found == runtime_cache.end()) {
        kernel = Shader::CreateKernelFromMemory(params, std::move(code));
    } else {
        kernel = Shader::CreateFromCache(params, found->second);
    }

    Shader* const result = kernel.get();
    if (cpu_addr) {
        Register(std::move(kernel), *cpu_addr, code_size);
    } else {
        null_kernel = std::move(kernel);
    }
    return result;
}

} // namespace OpenGL
