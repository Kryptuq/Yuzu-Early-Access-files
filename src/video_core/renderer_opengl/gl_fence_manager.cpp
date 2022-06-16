// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"

#include <glad/glad.h>

#include "video_core/renderer_opengl/gl_buffer_cache.h"
#include "video_core/renderer_opengl/gl_fence_manager.h"

namespace OpenGL {

GLInnerFence::GLInnerFence(bool is_stubbed_) : FenceBase{is_stubbed_} {}

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

Fence FenceManagerOpenGL::CreateFence(bool is_stubbed) {
    return std::make_shared<GLInnerFence>(is_stubbed);
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
