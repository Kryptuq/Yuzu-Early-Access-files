// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 460

out gl_PerVertex {
    vec4 gl_Position;
};

const vec2 vertices[4] =
    vec2[4](vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, -1.0));

layout (location = 0) out vec4 posPos;

#ifdef VULKAN

#define BINDING_COLOR_TEXTURE 0
#define VERTEX_ID gl_VertexIndex

#else // ^^^ Vulkan ^^^ // vvv OpenGL vvv

#define BINDING_COLOR_TEXTURE 0
#define VERTEX_ID gl_VertexID

#endif

layout (binding = BINDING_COLOR_TEXTURE) uniform sampler2D input_texture;

const float FXAA_SUBPIX_SHIFT = 0;

void main() {
  vec2 vertex = vertices[VERTEX_ID];
  gl_Position = vec4(vertex, 0.0, 1.0);
  vec2 vert_tex_coord = (vertex + 1.0) / 2.0;
  posPos.xy = vert_tex_coord;
  posPos.zw = vert_tex_coord - (0.5 + FXAA_SUBPIX_SHIFT) / textureSize(input_texture, 0);
}
