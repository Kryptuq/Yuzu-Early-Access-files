// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2013 Dolphin Emulator Project / 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include "common/common_types.h"

namespace Common {

/// x86/x64 CPU capabilities that may be detected by this module
struct CPUCaps {

    enum class Manufacturer : u8 {
        Unknown = 0,
        Intel = 1,
        AMD = 2,
        Hygon = 3,
    };

    static Manufacturer ParseManufacturer(std::string_view brand_string);

    Manufacturer manufacturer;
    char brand_string[13];

    char cpu_string[48];

    u32 base_frequency;
    u32 max_frequency;
    u32 bus_frequency;

    u32 tsc_crystal_ratio_denominator;
    u32 tsc_crystal_ratio_numerator;
    u32 crystal_frequency;
    u64 tsc_frequency; // Derived from the above three values

    bool sse : 1;
    bool sse2 : 1;
    bool sse3 : 1;
    bool ssse3 : 1;
    bool sse4_1 : 1;
    bool sse4_2 : 1;

    bool avx : 1;
    bool avx_vnni : 1;
    bool avx2 : 1;
    bool avx512f : 1;
    bool avx512dq : 1;
    bool avx512cd : 1;
    bool avx512bw : 1;
    bool avx512vl : 1;
    bool avx512vbmi : 1;
    bool avx512bitalg : 1;

    bool aes : 1;
    bool bmi1 : 1;
    bool bmi2 : 1;
    bool f16c : 1;
    bool fma : 1;
    bool fma4 : 1;
    bool gfni : 1;
    bool invariant_tsc : 1;
    bool lzcnt : 1;
    bool movbe : 1;
    bool pclmulqdq : 1;
    bool popcnt : 1;
    bool sha : 1;
};

/**
 * Gets the supported capabilities of the host CPU
 * @return Reference to a CPUCaps struct with the detected host CPU capabilities
 */
const CPUCaps& GetCPUCaps();

} // namespace Common
