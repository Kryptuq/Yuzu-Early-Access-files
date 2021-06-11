// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string_view>
#include <vector>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/renderer_opengl/gl_shader_util.h"

namespace OpenGL::GLShader {

namespace {

std::string_view StageDebugName(GLenum type) {
    switch (type) {
    case GL_VERTEX_SHADER:
        return "vertex";
    case GL_GEOMETRY_SHADER:
        return "geometry";
    case GL_FRAGMENT_SHADER:
        return "fragment";
    case GL_COMPUTE_SHADER:
        return "compute";
    }
    UNIMPLEMENTED();
    return "unknown";
}

} // Anonymous namespace

GLuint LoadShader(std::string_view source, GLenum type) {
    const std::string_view debug_type = StageDebugName(type);
    const GLuint shader_id = glCreateShader(type);

    const GLchar* source_string = source.data();
    const GLint source_length = static_cast<GLint>(source.size());

    glShaderSource(shader_id, 1, &source_string, &source_length);
    LOG_DEBUG(Render_OpenGL, "Compiling {} shader...", debug_type);
    glCompileShader(shader_id);

    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::string shader_error(info_log_length, ' ');
        glGetShaderInfoLog(shader_id, info_log_length, nullptr, &shader_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "{}", shader_error);
        } else {
            LOG_ERROR(Render_OpenGL, "Error compiling {} shader:\n{}", debug_type, shader_error);
        }
    }
    return shader_id;
}

} // namespace OpenGL::GLShader
