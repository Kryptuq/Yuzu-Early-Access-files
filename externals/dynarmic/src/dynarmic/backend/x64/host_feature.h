/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/stdint.hpp>

namespace Dynarmic::Backend::X64 {

enum class HostFeature : u64 {
    SSSE3 = 1ULL << 0,
    SSE41 = 1ULL << 1,
    SSE42 = 1ULL << 2,
    AVX = 1ULL << 3,
    AVX2 = 1ULL << 4,
    AVX512F = 1ULL << 5,
    AVX512CD = 1ULL << 6,
    AVX512VL = 1ULL << 7,
    AVX512BW = 1ULL << 8,
    AVX512DQ = 1ULL << 9,
    AVX512BITALG = 1ULL << 10,
    AVX512VBMI = 1ULL << 11,
    PCLMULQDQ = 1ULL << 12,
    F16C = 1ULL << 13,
    FMA = 1ULL << 14,
    AES = 1ULL << 15,
    SHA = 1ULL << 16,
    POPCNT = 1ULL << 17,
    BMI1 = 1ULL << 18,
    BMI2 = 1ULL << 19,
    LZCNT = 1ULL << 20,
    GFNI = 1ULL << 21,

    // Zen-based BMI2
    FastBMI2 = 1ULL << 22,

    // Orthographic AVX512 features on 128 and 256 vectors
    AVX512_Ortho = AVX512F | AVX512VL,

    // Orthographic AVX512 features for both 32-bit and 64-bit floats
    AVX512_OrthoFloat = AVX512_Ortho | AVX512DQ,
};

constexpr HostFeature operator~(HostFeature f) {
    return static_cast<HostFeature>(~static_cast<u64>(f));
}

constexpr HostFeature operator|(HostFeature f1, HostFeature f2) {
    return static_cast<HostFeature>(static_cast<u64>(f1) | static_cast<u64>(f2));
}

constexpr HostFeature operator&(HostFeature f1, HostFeature f2) {
    return static_cast<HostFeature>(static_cast<u64>(f1) & static_cast<u64>(f2));
}

constexpr HostFeature operator|=(HostFeature& result, HostFeature f) {
    return result = (result | f);
}

constexpr HostFeature operator&=(HostFeature& result, HostFeature f) {
    return result = (result & f);
}

}  // namespace Dynarmic::Backend::X64
