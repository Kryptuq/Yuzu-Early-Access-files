// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "common/common_types.h"
#include "video_core/fence_manager.h"
#include "video_core/renderer_opengl/gl_buffer_cache.h"
#include "video_core/renderer_opengl/gl_query_cache.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_texture_cache.h"

namespace OpenGL {

class GLInnerFence : public VideoCommon::FenceBase {
public:
    explicit GLInnerFence(u32 payload_, bool is_stubbed_);
    explicit GLInnerFence(GPUVAddr address_, u32 payload_, bool is_stubbed_);
    ~GLInnerFence();

    void Queue();

    bool IsSignaled() const;

    void Wait();

private:
    OGLSync sync_object;
};

using Fence = std::shared_ptr<GLInnerFence>;
using GenericFenceManager = VideoCommon::FenceManager<Fence, TextureCache, BufferCache, QueryCache>;

class FenceManagerOpenGL final : public GenericFenceManager {
public:
    explicit FenceManagerOpenGL(VideoCore::RasterizerInterface& rasterizer, Tegra::GPU& gpu,
                                TextureCache& texture_cache, BufferCache& buffer_cache,
                                QueryCache& query_cache);

protected:
    Fence CreateFence(u32 value, bool is_stubbed) override;
    Fence CreateFence(GPUVAddr addr, u32 value, bool is_stubbed) override;
    void QueueFence(Fence& fence) override;
    bool IsFenceSignaled(Fence& fence) const override;
    void WaitFence(Fence& fence) override;
};

} // namespace OpenGL
