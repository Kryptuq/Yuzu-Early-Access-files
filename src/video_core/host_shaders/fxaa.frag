// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// Source code is adapted from
// https://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/3/

#version 460

#ifdef VULKAN

#define BINDING_COLOR_TEXTURE 1

#else // ^^^ Vulkan ^^^ // vvv OpenGL vvv

#define BINDING_COLOR_TEXTURE 0

#endif

layout (location = 0) in vec4 posPos;

layout (location = 0) out vec4 frag_color;

layout (binding = BINDING_COLOR_TEXTURE) uniform sampler2D input_texture;

const float FXAA_SPAN_MAX = 8.0;
const float FXAA_REDUCE_MUL = 1.0 / 8.0;
const float FXAA_REDUCE_MIN = 1.0 / 128.0;

#define FxaaTexLod0(t, p) textureLod(t, p, 0.0)
#define FxaaTexOff(t, p, o) textureLodOffset(t, p, 0.0, o)

vec3 FxaaPixelShader(vec4 posPos, sampler2D tex) {

    vec3 rgbNW = FxaaTexLod0(tex, posPos.zw).xyz;
    vec3 rgbNE = FxaaTexOff(tex, posPos.zw, ivec2(1,0)).xyz;
    vec3 rgbSW = FxaaTexOff(tex, posPos.zw, ivec2(0,1)).xyz;
    vec3 rgbSE = FxaaTexOff(tex, posPos.zw, ivec2(1,1)).xyz;
    vec3 rgbM  = FxaaTexLod0(tex, posPos.xy).xyz;
/*---------------------------------------------------------*/
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
/*---------------------------------------------------------*/
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
/*---------------------------------------------------------*/
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
/*---------------------------------------------------------*/
    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) / textureSize(tex, 0);
/*--------------------------------------------------------*/
    vec3 rgbA = (1.0 / 2.0) * (
        FxaaTexLod0(tex, posPos.xy + dir * (1.0 / 3.0 - 0.5)).xyz +
        FxaaTexLod0(tex, posPos.xy + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0 / 2.0) + (1.0 / 4.0) * (
        FxaaTexLod0(tex, posPos.xy + dir * (0.0 / 3.0 - 0.5)).xyz +
        FxaaTexLod0(tex, posPos.xy + dir * (3.0 / 3.0 - 0.5)).xyz);
    float lumaB = dot(rgbB, luma);
    if((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;
    return rgbB;
}

void main() {
  frag_color = vec4(FxaaPixelShader(posPos, input_texture), 1.0);
}
