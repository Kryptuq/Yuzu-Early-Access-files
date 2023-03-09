// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include <utility>
#include <glad/glad.h>
#include "common/common_funcs.h"

namespace OpenGL {

class OGLRenderbuffer final {
public:
    YUZU_NON_COPYABLE(OGLRenderbuffer);

    OGLRenderbuffer() = default;

    OGLRenderbuffer(OGLRenderbuffer&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLRenderbuffer() {
        Release();
    }

    OGLRenderbuffer& operator=(OGLRenderbuffer&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLTexture final {
public:
    YUZU_NON_COPYABLE(OGLTexture);

    OGLTexture() = default;

    OGLTexture(OGLTexture&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLTexture() {
        Release();
    }

    OGLTexture& operator=(OGLTexture&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create(GLenum target);

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLTextureView final {
public:
    YUZU_NON_COPYABLE(OGLTextureView);

    OGLTextureView() = default;

    OGLTextureView(OGLTextureView&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLTextureView() {
        Release();
    }

    OGLTextureView& operator=(OGLTextureView&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLSampler final {
public:
    YUZU_NON_COPYABLE(OGLSampler);

    OGLSampler() = default;

    OGLSampler(OGLSampler&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLSampler() {
        Release();
    }

    OGLSampler& operator=(OGLSampler&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLShader final {
public:
    YUZU_NON_COPYABLE(OGLShader);

    OGLShader() = default;

    OGLShader(OGLShader&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLShader() {
        Release();
    }

    OGLShader& operator=(OGLShader&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    void Release();

    GLuint handle = 0;
};

class OGLProgram final {
public:
    YUZU_NON_COPYABLE(OGLProgram);

    OGLProgram() = default;

    OGLProgram(OGLProgram&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLProgram() {
        Release();
    }

    OGLProgram& operator=(OGLProgram&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLAssemblyProgram final {
public:
    YUZU_NON_COPYABLE(OGLAssemblyProgram);

    OGLAssemblyProgram() = default;

    OGLAssemblyProgram(OGLAssemblyProgram&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLAssemblyProgram() {
        Release();
    }

    OGLAssemblyProgram& operator=(OGLAssemblyProgram&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLPipeline final {
public:
    YUZU_NON_COPYABLE(OGLPipeline);

    OGLPipeline() = default;
    OGLPipeline(OGLPipeline&& o) noexcept : handle{std::exchange<GLuint>(o.handle, 0)} {}

    ~OGLPipeline() {
        Release();
    }
    OGLPipeline& operator=(OGLPipeline&& o) noexcept {
        handle = std::exchange<GLuint>(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLBuffer final {
public:
    YUZU_NON_COPYABLE(OGLBuffer);

    OGLBuffer() = default;

    OGLBuffer(OGLBuffer&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLBuffer() {
        Release();
    }

    OGLBuffer& operator=(OGLBuffer&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLSync final {
public:
    YUZU_NON_COPYABLE(OGLSync);

    OGLSync() = default;

    OGLSync(OGLSync&& o) noexcept : handle(std::exchange(o.handle, nullptr)) {}

    ~OGLSync() {
        Release();
    }
    OGLSync& operator=(OGLSync&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, nullptr);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    /// Checks if the sync has been signaled
    bool IsSignaled() const noexcept;

    GLsync handle = 0;
};

class OGLFramebuffer final {
public:
    YUZU_NON_COPYABLE(OGLFramebuffer);

    OGLFramebuffer() = default;

    OGLFramebuffer(OGLFramebuffer&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLFramebuffer() {
        Release();
    }

    OGLFramebuffer& operator=(OGLFramebuffer&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLQuery final {
public:
    YUZU_NON_COPYABLE(OGLQuery);

    OGLQuery() = default;

    OGLQuery(OGLQuery&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLQuery() {
        Release();
    }

    OGLQuery& operator=(OGLQuery&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create(GLenum target);

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

} // namespace OpenGL
