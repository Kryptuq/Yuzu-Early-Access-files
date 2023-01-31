// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <unordered_map>

#include "common/common_types.h"
#include "common/thread_worker.h"
#include "shader_recompiler/host_translate_info.h"
#include "shader_recompiler/profile.h"
#include "video_core/renderer_opengl/gl_compute_pipeline.h"
#include "video_core/renderer_opengl/gl_graphics_pipeline.h"
#include "video_core/renderer_opengl/gl_shader_context.h"
#include "video_core/shader_cache.h"

namespace Tegra {
class MemoryManager;
}

namespace OpenGL {

class Device;
class ProgramManager;
class RasterizerOpenGL;
using ShaderWorker = Common::StatefulThreadWorker<ShaderContext::Context>;

class ShaderCache : public VideoCommon::ShaderCache {
public:
    explicit ShaderCache(RasterizerOpenGL& rasterizer_, Core::Frontend::EmuWindow& emu_window_,
                         const Device& device_, TextureCache& texture_cache_,
                         BufferCache& buffer_cache_, ProgramManager& program_manager_,
                         StateTracker& state_tracker_, VideoCore::ShaderNotify& shader_notify_);
    ~ShaderCache();

    void LoadDiskResources(u64 title_id, std::stop_token stop_loading,
                           const VideoCore::DiskResourceLoadCallback& callback);

    [[nodiscard]] GraphicsPipeline* CurrentGraphicsPipeline();

    [[nodiscard]] ComputePipeline* CurrentComputePipeline();

private:
    GraphicsPipeline* CurrentGraphicsPipelineSlowPath();

    [[nodiscard]] GraphicsPipeline* BuiltPipeline(GraphicsPipeline* pipeline) const noexcept;

    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline();

    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(
        ShaderContext::ShaderPools& pools, const GraphicsPipelineKey& key,
        std::span<Shader::Environment* const> envs, bool use_shader_workers,
        bool force_context_flush = false);

    std::unique_ptr<ComputePipeline> CreateComputePipeline(const ComputePipelineKey& key,
                                                           const VideoCommon::ShaderInfo* shader);

    std::unique_ptr<ComputePipeline> CreateComputePipeline(ShaderContext::ShaderPools& pools,
                                                           const ComputePipelineKey& key,
                                                           Shader::Environment& env,
                                                           bool force_context_flush = false);

    std::unique_ptr<ShaderWorker> CreateWorkers() const;

    Core::Frontend::EmuWindow& emu_window;
    const Device& device;
    TextureCache& texture_cache;
    BufferCache& buffer_cache;
    ProgramManager& program_manager;
    StateTracker& state_tracker;
    VideoCore::ShaderNotify& shader_notify;
    const bool use_asynchronous_shaders;
    const bool strict_context_required;

    GraphicsPipelineKey graphics_key{};
    GraphicsPipeline* current_pipeline{};

    ShaderContext::ShaderPools main_pools;
    std::unordered_map<GraphicsPipelineKey, std::unique_ptr<GraphicsPipeline>> graphics_cache;
    std::unordered_map<ComputePipelineKey, std::unique_ptr<ComputePipeline>> compute_cache;

    Shader::Profile profile;
    Shader::HostTranslateInfo host_info;

    std::filesystem::path shader_cache_filename;
    std::unique_ptr<ShaderWorker> workers;
};

} // namespace OpenGL
