// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>
#include <utility>

#include <glad/glad.h>

#include "common/common_types.h"
#include "common/literals.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"

namespace OpenGL {

using namespace Common::Literals;

class StreamBuffer {
    static constexpr size_t STREAM_BUFFER_SIZE = 64_MiB;
    static constexpr size_t NUM_SYNCS = 16;
    static constexpr size_t REGION_SIZE = STREAM_BUFFER_SIZE / NUM_SYNCS;
    static constexpr size_t MAX_ALIGNMENT = 256;
    static_assert(STREAM_BUFFER_SIZE % MAX_ALIGNMENT == 0);
    static_assert(STREAM_BUFFER_SIZE % NUM_SYNCS == 0);
    static_assert(REGION_SIZE % MAX_ALIGNMENT == 0);

public:
    explicit StreamBuffer();

    [[nodiscard]] std::pair<std::span<u8>, size_t> Request(size_t size) noexcept;

    [[nodiscard]] GLuint Handle() const noexcept {
        return buffer.handle;
    }

private:
    [[nodiscard]] static size_t Region(size_t offset) noexcept {
        return offset / REGION_SIZE;
    }

    size_t iterator = 0;
    size_t used_iterator = 0;
    size_t free_iterator = 0;
    u8* mapped_pointer = nullptr;
    OGLBuffer buffer;
    std::array<OGLSync, NUM_SYNCS> fences;
};

} // namespace OpenGL
