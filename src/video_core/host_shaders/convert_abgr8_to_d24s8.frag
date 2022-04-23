// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#version 450
#extension GL_ARB_shader_stencil_export : require

layout(binding = 0) uniform sampler2D color_texture;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uvec4 color = uvec4(texelFetch(color_texture, coord, 0).abgr * (exp2(8) - 1.0f));
    uvec4 bytes = color << uvec4(24, 16, 8, 0);
    uint depth_stencil_unorm = bytes.x | bytes.y | bytes.z | bytes.w;

    gl_FragDepth = float(depth_stencil_unorm & 0x00FFFFFFu) / (exp2(24.0) - 1.0f);
    gl_FragStencilRefARB = int(depth_stencil_unorm >> 24);
}
