// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 450
#extension GL_ARB_shader_stencil_export : require

layout(binding = 0) uniform sampler2D color_texture;

uint conv_from_float(float value_f, uint mantissa_bits) {
    uint value = floatBitsToInt(value_f);
    uint exp = (value >> 23) & 0x1Fu;
    uint mantissa_shift = 32u - mantissa_bits;
    uint mantissa = (value << 9u) >> mantissa_shift;
    return (exp << mantissa_bits) | mantissa;
}

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 color = texelFetch(color_texture, coord, 0).rgba;
    uint depth_stencil_unorm = (conv_from_float(color.r, 6u) << 21)
                      | (conv_from_float(color.g, 6u) << 10)
                      | conv_from_float(color.b, 5u);

    gl_FragDepth = float(depth_stencil_unorm & 0x00FFFFFFu) / (exp2(24.0) - 1.0f);
    gl_FragStencilRefARB = int(depth_stencil_unorm >> 24);
}
