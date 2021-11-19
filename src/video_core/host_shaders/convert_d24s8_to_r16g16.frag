// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 450

layout(binding = 0) uniform sampler2D depth_tex;
layout(binding = 1) uniform isampler2D stencil_tex;

layout(location = 0) out vec4 color;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uint depth = uint(textureLod(depth_tex, coord, 0).r * (exp2(24.0) - 1.0f));
    uint stencil = uint(textureLod(stencil_tex, coord, 0).r);

    color.r = float(depth >> 16) / (exp2(16) - 1.0);
    color.g = float((depth >> 16) & 0x00FF) / (exp2(16) - 1.0);
    color.b = 0.0f;
    color.a = 1.0f;
}
