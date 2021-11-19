// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 450
// #extension GL_ARB_shader_stencil_export : require

layout(binding = 0) uniform sampler2D color_texture;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 color = texelFetch(color_texture, coord, 0).rgba;
    uint depth_stencil_unorm = (uint(color.b * (exp2(10) - 1.0f)) << 22)
                      | (uint(color.g * (exp2(11) - 1.0f)) << 11)
                      | (uint(color.r * (exp2(11) - 1.0f)));

    gl_FragDepth = float(depth_stencil_unorm >> 8) / (exp2(24.0) - 1.0f);
    // gl_FragStencilRefARB = int(depth_stencil_unorm & 0x00FF);
}
