// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 460 core

layout (location = 0) in vec2 frag_tex_coord;

layout (location = 0) out vec4 color;

layout (binding = 1) uniform sampler2D color_texture;

void main() {
    color = texture(color_texture, frag_tex_coord);
}
