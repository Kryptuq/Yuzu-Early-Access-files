// Copyright 2013 Dolphin Emulator Project / 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Common {

enum class Manufacturer : u32 {
    Intel = 0,
    AMD = 1,
    Hygon = 2,
    Unknown = 3,
};

/// x86/x64 CPU capabilities that may be detected by this module
struct CPUCaps {
    Manufacturer manufacturer;
    char cpu_string[0x21];
    char brand_string[0x41];
    bool sse;
    bool sse2;
    bool sse3;
    bool ssse3;
    bool sse4_1;
    bool sse4_2;
    bool lzcnt;
    bool avx;
    bool avx2;
    bool avx512;
    bool bmi1;
    bool bmi2;
    bool fma;
    bool fma4;
    bool aes;
    bool invariant_tsc;
    u32 base_frequency;
    u32 max_frequency;
    u32 bus_frequency;
};

/**
 * Gets the supported capabilities of the host CPU
 * @return Reference to a CPUCaps struct with the detected host CPU capabilities
 */
const CPUCaps& GetCPUCaps();

} // namespace Common
