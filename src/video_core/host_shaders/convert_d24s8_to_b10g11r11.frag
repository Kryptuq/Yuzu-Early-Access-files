// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 450

layout(binding = 0) uniform sampler2D depth_tex;
layout(binding = 1) uniform isampler2D stencil_tex;

layout(location = 0) out vec4 color;

float conv_to_float(uint value, uint mantissa_bits) {
    uint exp = (value >> mantissa_bits) & 0x1Fu;
    uint mantissa_shift = 32u - mantissa_bits;
    uint mantissa = (value << mantissa_shift) >> mantissa_shift;
    return uintBitsToFloat((exp << 23) | (mantissa << (23 - mantissa_bits)));
}

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uint depth = uint(textureLod(depth_tex, coord, 0).r * (exp2(32.0) - 1.0f));
    uint stencil = uint(textureLod(stencil_tex, coord, 0).r);
    uint depth_stencil = (stencil << 24) | (depth >> 8);
    uint red_int = (depth_stencil >> 21) & 0x07FF;
    uint green_int = (depth_stencil >> 10) & 0x07FF;
    uint blue_int = depth_stencil & 0x03FF;

    color.r = conv_to_float(red_int, 6u);
    color.g = conv_to_float(green_int, 6u);
    color.b = conv_to_float(blue_int, 5u);
    color.a = 1.0f;
}
