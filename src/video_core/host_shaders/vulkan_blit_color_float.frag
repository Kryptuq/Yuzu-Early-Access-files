// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#version 450

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 color;

void main() {
    color = textureLod(tex, texcoord, 0);
}
