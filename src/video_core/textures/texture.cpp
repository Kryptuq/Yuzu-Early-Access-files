// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include "common/cityhash.h"
#include "common/settings.h"
#include "video_core/textures/texture.h"

using Tegra::Texture::TICEntry;
using Tegra::Texture::TSCEntry;

namespace Tegra::Texture {

namespace {

constexpr std::array<float, 256> SRGB_CONVERSION_LUT = {
    0.000000f, 0.000000f, 0.000000f, 0.000012f, 0.000021f, 0.000033f, 0.000046f, 0.000062f,
    0.000081f, 0.000102f, 0.000125f, 0.000151f, 0.000181f, 0.000214f, 0.000251f, 0.000293f,
    0.000338f, 0.000388f, 0.000443f, 0.000503f, 0.000568f, 0.000639f, 0.000715f, 0.000798f,
    0.000887f, 0.000983f, 0.001085f, 0.001195f, 0.001312f, 0.001437f, 0.001569f, 0.001710f,
    0.001860f, 0.002019f, 0.002186f, 0.002364f, 0.002551f, 0.002748f, 0.002955f, 0.003174f,
    0.003403f, 0.003643f, 0.003896f, 0.004160f, 0.004436f, 0.004725f, 0.005028f, 0.005343f,
    0.005672f, 0.006015f, 0.006372f, 0.006744f, 0.007130f, 0.007533f, 0.007950f, 0.008384f,
    0.008834f, 0.009301f, 0.009785f, 0.010286f, 0.010805f, 0.011342f, 0.011898f, 0.012472f,
    0.013066f, 0.013680f, 0.014313f, 0.014967f, 0.015641f, 0.016337f, 0.017054f, 0.017793f,
    0.018554f, 0.019337f, 0.020144f, 0.020974f, 0.021828f, 0.022706f, 0.023609f, 0.024536f,
    0.025489f, 0.026468f, 0.027473f, 0.028504f, 0.029563f, 0.030649f, 0.031762f, 0.032904f,
    0.034074f, 0.035274f, 0.036503f, 0.037762f, 0.039050f, 0.040370f, 0.041721f, 0.043103f,
    0.044518f, 0.045964f, 0.047444f, 0.048956f, 0.050503f, 0.052083f, 0.053699f, 0.055349f,
    0.057034f, 0.058755f, 0.060513f, 0.062307f, 0.064139f, 0.066008f, 0.067915f, 0.069861f,
    0.071845f, 0.073869f, 0.075933f, 0.078037f, 0.080182f, 0.082369f, 0.084597f, 0.086867f,
    0.089180f, 0.091535f, 0.093935f, 0.096378f, 0.098866f, 0.101398f, 0.103977f, 0.106601f,
    0.109271f, 0.111988f, 0.114753f, 0.117565f, 0.120426f, 0.123335f, 0.126293f, 0.129301f,
    0.132360f, 0.135469f, 0.138629f, 0.141841f, 0.145105f, 0.148421f, 0.151791f, 0.155214f,
    0.158691f, 0.162224f, 0.165810f, 0.169453f, 0.173152f, 0.176907f, 0.180720f, 0.184589f,
    0.188517f, 0.192504f, 0.196549f, 0.200655f, 0.204820f, 0.209046f, 0.213334f, 0.217682f,
    0.222093f, 0.226567f, 0.231104f, 0.235704f, 0.240369f, 0.245099f, 0.249894f, 0.254754f,
    0.259681f, 0.264674f, 0.269736f, 0.274864f, 0.280062f, 0.285328f, 0.290664f, 0.296070f,
    0.301546f, 0.307094f, 0.312713f, 0.318404f, 0.324168f, 0.330006f, 0.335916f, 0.341902f,
    0.347962f, 0.354097f, 0.360309f, 0.366597f, 0.372961f, 0.379403f, 0.385924f, 0.392524f,
    0.399202f, 0.405960f, 0.412798f, 0.419718f, 0.426719f, 0.433802f, 0.440967f, 0.448216f,
    0.455548f, 0.462965f, 0.470465f, 0.478052f, 0.485725f, 0.493484f, 0.501329f, 0.509263f,
    0.517285f, 0.525396f, 0.533595f, 0.541885f, 0.550265f, 0.558736f, 0.567299f, 0.575954f,
    0.584702f, 0.593542f, 0.602477f, 0.611507f, 0.620632f, 0.629852f, 0.639168f, 0.648581f,
    0.658092f, 0.667700f, 0.677408f, 0.687214f, 0.697120f, 0.707127f, 0.717234f, 0.727443f,
    0.737753f, 0.748167f, 0.758685f, 0.769305f, 0.780031f, 0.790861f, 0.801798f, 0.812839f,
    0.823989f, 0.835246f, 0.846611f, 0.858085f, 0.869668f, 0.881360f, 0.893164f, 0.905078f,
    0.917104f, 0.929242f, 0.941493f, 0.953859f, 0.966338f, 1.000000f, 1.000000f, 1.000000f,
};

} // Anonymous namespace

std::array<float, 4> TSCEntry::BorderColor() const noexcept {
    if (!srgb_conversion) {
        return border_color;
    }
    return {SRGB_CONVERSION_LUT[srgb_border_color_r], SRGB_CONVERSION_LUT[srgb_border_color_g],
            SRGB_CONVERSION_LUT[srgb_border_color_b], border_color[3]};
}

float TSCEntry::MaxAnisotropy() const noexcept {
    if (max_anisotropy == 0 && mipmap_filter != TextureMipmapFilter::Linear) {
        return 1.0f;
    }
    const auto anisotropic_settings = Settings::values.max_anisotropy.GetValue();
    u32 added_anisotropic{};
    if (anisotropic_settings == 0) {
        added_anisotropic = Settings::values.resolution_info.up_scale >>
                            Settings::values.resolution_info.down_shift;
    } else {
        added_anisotropic = Settings::values.max_anisotropy.GetValue() - 1U;
    }
    return static_cast<float>(1U << (max_anisotropy + added_anisotropic));
}

} // namespace Tegra::Texture

size_t std::hash<TICEntry>::operator()(const TICEntry& tic) const noexcept {
    return Common::CityHash64(reinterpret_cast<const char*>(&tic), sizeof tic);
}

size_t std::hash<TSCEntry>::operator()(const TSCEntry& tsc) const noexcept {
    return Common::CityHash64(reinterpret_cast<const char*>(&tsc), sizeof tsc);
}
