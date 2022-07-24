// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#version 450

layout(binding = 0) uniform sampler2D color_texture;

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    float color = texelFetch(color_texture, coord, 0).r;
    gl_FragDepth = color;
}
