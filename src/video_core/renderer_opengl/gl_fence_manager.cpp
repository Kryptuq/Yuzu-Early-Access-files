// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"

#include <glad/glad.h>

#include "video_core/renderer_opengl/gl_buffer_cache.h"
#include "video_core/renderer_opengl/gl_fence_manager.h"

namespace OpenGL {

GLInnerFence::GLInnerFence(u32 payload_, bool is_stubbed_) : FenceBase{payload_, is_stubbed_} {}

GLInnerFence::GLInnerFence(GPUVAddr address_, u32 payload_, bool is_stubbed_)
    : FenceBase{address_, payload_, is_stubbed_} {}

GLInnerFence::~GLInnerFence() = default;

void GLInnerFence::Queue() {
    if (is_stubbed) {
        return;
    }
    ASSERT(sync_object.handle == 0);
    sync_object.Create();
}

bool GLInnerFence::IsSignaled() const {
    if (is_stubbed) {
        return true;
    }
    ASSERT(sync_object.handle != 0);
    GLint sync_status;
    glGetSynciv(sync_object.handle, GL_SYNC_STATUS, 1, nullptr, &sync_status);
    return sync_status == GL_SIGNALED;
}

void GLInnerFence::Wait() {
    if (is_stubbed) {
        return;
    }
    ASSERT(sync_object.handle != 0);
    glClientWaitSync(sync_object.handle, 0, GL_TIMEOUT_IGNORED);
}

FenceManagerOpenGL::FenceManagerOpenGL(VideoCore::RasterizerInterface& rasterizer_,
                                       Tegra::GPU& gpu_, TextureCache& texture_cache_,
                                       BufferCache& buffer_cache_, QueryCache& query_cache_)
    : GenericFenceManager{rasterizer_, gpu_, texture_cache_, buffer_cache_, query_cache_} {}

Fence FenceManagerOpenGL::CreateFence(u32 value, bool is_stubbed) {
    return std::make_shared<GLInnerFence>(value, is_stubbed);
}

Fence FenceManagerOpenGL::CreateFence(GPUVAddr addr, u32 value, bool is_stubbed) {
    return std::make_shared<GLInnerFence>(addr, value, is_stubbed);
}

void FenceManagerOpenGL::QueueFence(Fence& fence) {
    fence->Queue();
}

bool FenceManagerOpenGL::IsFenceSignaled(Fence& fence) const {
    return fence->IsSignaled();
}

void FenceManagerOpenGL::WaitFence(Fence& fence) {
    fence->Wait();
}

} // namespace OpenGL
