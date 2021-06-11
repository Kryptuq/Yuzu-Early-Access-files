// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/logging/log.h"

namespace OpenGL::GLShader {

/**
 * Utility function to log the source code of a list of shaders.
 * @param shaders The OpenGL shaders whose source we will print.
 */
template <typename... T>
void LogShaderSource(T... shaders) {
    auto shader_list = {shaders...};

    for (const auto& shader : shader_list) {
        if (shader == 0)
            continue;

        GLint source_length;
        glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &source_length);

        std::string source(source_length, ' ');
        glGetShaderSource(shader, source_length, nullptr, &source[0]);
        LOG_INFO(Render_OpenGL, "Shader source {}", source);
    }
}

/**
 * Utility function to create and compile an OpenGL GLSL shader
 * @param source String of the GLSL shader program
 * @param type Type of the shader (GL_VERTEX_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER)
 */
GLuint LoadShader(std::string_view source, GLenum type);

/**
 * Utility function to create and compile an OpenGL GLSL shader program (vertex + fragment shader)
 * @param separable_program whether to create a separable program
 * @param shaders ID of shaders to attach to the program
 * @returns Handle of the newly created OpenGL program object
 */
template <typename... T>
GLuint LoadProgram(bool separable_program, bool hint_retrievable, T... shaders) {
    // Link the program
    LOG_DEBUG(Render_OpenGL, "Linking program...");

    GLuint program_id = glCreateProgram();

    ((shaders == 0 ? (void)0 : glAttachShader(program_id, shaders)), ...);

    if (separable_program) {
        glProgramParameteri(program_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
    }
    if (hint_retrievable) {
        glProgramParameteri(program_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    }

    glLinkProgram(program_id);

    // Check the program
    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::string program_error(info_log_length, ' ');
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "{}", program_error);
        } else {
            LOG_ERROR(Render_OpenGL, "Error linking shader:\n{}", program_error);
        }
    }

    if (result == GL_FALSE) {
        // There was a problem linking the shader, print the source for debugging purposes.
        LogShaderSource(shaders...);
    }

    ASSERT_MSG(result == GL_TRUE, "Shader not linked");

    ((shaders == 0 ? (void)0 : glDetachShader(program_id, shaders)), ...);

    return program_id;
}

} // namespace OpenGL::GLShader
