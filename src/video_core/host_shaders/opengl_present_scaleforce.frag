// SPDX-FileCopyrightText: 2020 BreadFish64
// SPDX-License-Identifier: MIT

// Adapted from https://github.com/BreadFish64/ScaleFish/tree/master/scaleforce

//! #version 460

#extension GL_ARB_separate_shader_objects : enable

#ifdef YUZU_USE_FP16

#extension GL_AMD_gpu_shader_half_float : enable
#extension GL_NV_gpu_shader5 : enable

#define lfloat float16_t
#define lvec2 f16vec2
#define lvec3 f16vec3
#define lvec4 f16vec4

#else

#define lfloat float
#define lvec2 vec2
#define lvec3 vec3
#define lvec4 vec4

#endif

#ifdef VULKAN

#define BINDING_COLOR_TEXTURE 1

#else // ^^^ Vulkan ^^^ // vvv OpenGL vvv

#define BINDING_COLOR_TEXTURE 0

#endif

layout (location = 0) in vec2 tex_coord;

layout (location = 0) out vec4 frag_color;

layout (binding = BINDING_COLOR_TEXTURE) uniform sampler2D input_texture;

const bool ignore_alpha = true;

lfloat ColorDist1(lvec4 a, lvec4 b) {
    // https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.2020_conversion
    const lvec3 K = lvec3(0.2627, 0.6780, 0.0593);
    const lfloat scaleB = lfloat(0.5) / (lfloat(1.0) - K.b);
    const lfloat scaleR = lfloat(0.5) / (lfloat(1.0) - K.r);
    lvec4 diff = a - b;
    lfloat Y = dot(diff.rgb, K);
    lfloat Cb = scaleB * (diff.b - Y);
    lfloat Cr = scaleR * (diff.r - Y);
    lvec3 YCbCr = lvec3(Y, Cb, Cr);
    lfloat d = length(YCbCr);
    if (ignore_alpha) {
        return d;
    }
    return sqrt(a.a * b.a * d * d + diff.a * diff.a);
}

lvec4 ColorDist(lvec4 ref, lvec4 A, lvec4 B, lvec4 C, lvec4 D) {
    return lvec4(
            ColorDist1(ref, A),
            ColorDist1(ref, B),
            ColorDist1(ref, C),
            ColorDist1(ref, D)
        );
}

vec4 Scaleforce(sampler2D tex, vec2 tex_coord) {
    lvec4 bl = lvec4(textureOffset(tex, tex_coord, ivec2(-1, -1)));
    lvec4 bc = lvec4(textureOffset(tex, tex_coord, ivec2(0, -1)));
    lvec4 br = lvec4(textureOffset(tex, tex_coord, ivec2(1, -1)));
    lvec4 cl = lvec4(textureOffset(tex, tex_coord, ivec2(-1, 0)));
    lvec4 cc = lvec4(texture(tex, tex_coord));
    lvec4 cr = lvec4(textureOffset(tex, tex_coord, ivec2(1, 0)));
    lvec4 tl = lvec4(textureOffset(tex, tex_coord, ivec2(-1, 1)));
    lvec4 tc = lvec4(textureOffset(tex, tex_coord, ivec2(0, 1)));
    lvec4 tr = lvec4(textureOffset(tex, tex_coord, ivec2(1, 1)));

    lvec4 offset_tl = ColorDist(cc, tl, tc, tr, cr);
    lvec4 offset_br = ColorDist(cc, br, bc, bl, cl);

    // Calculate how different cc is from the texels around it
    const lfloat plus_weight = lfloat(1.5);
    const lfloat cross_weight = lfloat(1.5);
    lfloat total_dist = dot(offset_tl + offset_br, lvec4(cross_weight, plus_weight, cross_weight, plus_weight));

    if (total_dist == lfloat(0.0)) {
        return cc;
    } else {
        // Add together all the distances with direction taken into account
        lvec4 tmp = offset_tl - offset_br;
        lvec2 total_offset = tmp.wy * plus_weight + (tmp.zz + lvec2(-tmp.x, tmp.x)) * cross_weight;

        // When the image has thin points, they tend to split apart.
        // This is because the texels all around are different and total_offset reaches into clear areas.
        // This works pretty well to keep the offset in bounds for these cases.
        lfloat clamp_val = length(total_offset) / total_dist;
        vec2 final_offset = vec2(clamp(total_offset, -clamp_val, clamp_val)) / textureSize(tex, 0);

        return texture(tex, tex_coord - final_offset);
    }
}

void main() {
    frag_color = Scaleforce(input_texture, tex_coord);
}
