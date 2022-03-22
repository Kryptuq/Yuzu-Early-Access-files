// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <span>
#include <string_view>

#include <glad/glad.h>

#include "video_core/renderer_opengl/gl_resource_manager.h"

namespace OpenGL {

OGLProgram CreateProgram(std::string_view code, GLenum stage);

OGLProgram CreateProgram(std::span<const u32> code, GLenum stage);

OGLAssemblyProgram CompileProgram(std::string_view code, GLenum target);

} // namespace OpenGL
