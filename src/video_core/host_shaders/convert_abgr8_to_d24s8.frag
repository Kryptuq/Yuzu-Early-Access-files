// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 450
// #extension GL_ARB_shader_stencil_export : require

layout(binding = 0) uniform sampler2D color_texture;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uvec4 color = uvec4(texelFetch(color_texture, coord, 0).rgba * (exp2(8) - 1.0f));
    uint depth_unorm = (color.r << 16) | (color.g << 8) | color.b;

    gl_FragDepth = float(depth_unorm) / (exp2(24.0) - 1.0f);
    // gl_FragStencilRefARB = int(color.a);
}
