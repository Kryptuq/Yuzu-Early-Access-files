// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#version 460 core

#ifdef VULKAN

#define BINDING_COLOR_TEXTURE 1

#else // ^^^ Vulkan ^^^ // vvv OpenGL vvv

#define BINDING_COLOR_TEXTURE 0

#endif


layout (location = 0) in vec2 frag_tex_coord;

layout (location = 0) out vec4 color;

layout (binding = BINDING_COLOR_TEXTURE) uniform sampler2D color_texture;

vec4 cubic(float v) {
    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return vec4(x, y, z, w) * (1.0 / 6.0);
}

vec4 textureBicubic( sampler2D textureSampler, vec2 texCoords ) {

    vec2 texSize = textureSize(textureSampler, 0);
    vec2 invTexSize = 1.0 / texSize;

    texCoords = texCoords * texSize - 0.5;

    vec2 fxy = fract(texCoords);
    texCoords -= fxy;

    vec4 xcubic = cubic(fxy.x);
    vec4 ycubic = cubic(fxy.y);

    vec4 c = texCoords.xxyy + vec2(-0.5, +1.5).xyxy;

    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4(xcubic.yw, ycubic.yw) / s;

    offset *= invTexSize.xxyy;

    vec4 sample0 = texture(textureSampler, offset.xz);
    vec4 sample1 = texture(textureSampler, offset.yz);
    vec4 sample2 = texture(textureSampler, offset.xw);
    vec4 sample3 = texture(textureSampler, offset.yw);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

void main() {
    color = vec4(textureBicubic(color_texture, frag_tex_coord).rgb, 1.0f);
}
