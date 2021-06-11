// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 450

layout(binding = 0) uniform sampler2D depth_texture;
layout(location = 0) out float output_color;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    output_color = texelFetch(depth_texture, coord, 0).r;
}
