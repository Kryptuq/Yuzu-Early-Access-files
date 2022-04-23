// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#version 450

layout(binding = 0) uniform sampler2D depth_tex;
layout(binding = 1) uniform isampler2D stencil_tex;

layout(location = 0) out vec4 color;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uint depth = uint(textureLod(depth_tex, coord, 0).r * (exp2(24.0) - 1.0f));
    uint stencil = uint(textureLod(stencil_tex, coord, 0).r);

    highp uint depth_val =
        uint(textureLod(depth_tex, coord, 0).r * (exp2(32.0) - 1.0));
    lowp uint stencil_val = textureLod(stencil_tex, coord, 0).r;
    highp uvec4 components =
        uvec4((uvec3(depth_val) >> uvec3(24u, 16u, 8u)) & 0x000000FFu, stencil_val);
    color.rgba = vec4(components) / (exp2(8.0) - 1.0);
}
