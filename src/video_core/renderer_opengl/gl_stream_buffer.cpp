// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <memory>
#include <span>

#include <glad/glad.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "video_core/renderer_opengl/gl_stream_buffer.h"

namespace OpenGL {

StreamBuffer::StreamBuffer() {
    static constexpr GLenum flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    buffer.Create();
    glObjectLabel(GL_BUFFER, buffer.handle, -1, "Stream Buffer");
    glNamedBufferStorage(buffer.handle, STREAM_BUFFER_SIZE, nullptr, flags);
    mapped_pointer =
        static_cast<u8*>(glMapNamedBufferRange(buffer.handle, 0, STREAM_BUFFER_SIZE, flags));
    for (OGLSync& sync : fences) {
        sync.Create();
    }
}

std::pair<std::span<u8>, size_t> StreamBuffer::Request(size_t size) noexcept {
    ASSERT(size < REGION_SIZE);
    for (size_t region = Region(used_iterator), region_end = Region(iterator); region < region_end;
         ++region) {
        fences[region].Create();
    }
    used_iterator = iterator;

    for (size_t region = Region(free_iterator) + 1,
                region_end = std::min(Region(iterator + size) + 1, NUM_SYNCS);
         region < region_end; ++region) {
        glClientWaitSync(fences[region].handle, 0, GL_TIMEOUT_IGNORED);
        fences[region].Release();
    }
    if (iterator + size >= free_iterator) {
        free_iterator = iterator + size;
    }
    if (iterator + size > STREAM_BUFFER_SIZE) {
        for (size_t region = Region(used_iterator); region < NUM_SYNCS; ++region) {
            fences[region].Create();
        }
        used_iterator = 0;
        iterator = 0;
        free_iterator = size;

        for (size_t region = 0, region_end = Region(size); region <= region_end; ++region) {
            glClientWaitSync(fences[region].handle, 0, GL_TIMEOUT_IGNORED);
            fences[region].Release();
        }
    }
    const size_t offset = iterator;
    iterator = Common::AlignUp(iterator + size, MAX_ALIGNMENT);
    return {std::span(mapped_pointer + offset, size), offset};
}

} // namespace OpenGL
