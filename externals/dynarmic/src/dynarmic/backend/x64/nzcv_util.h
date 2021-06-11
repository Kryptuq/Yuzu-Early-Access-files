/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"

namespace Dynarmic::Backend::X64::NZCV {

constexpr u32 arm_mask = 0xF000'0000;
constexpr u32 x64_mask = 0xC101;

constexpr size_t x64_n_flag_bit = 15;
constexpr size_t x64_z_flag_bit = 14;
constexpr size_t x64_c_flag_bit = 8;
constexpr size_t x64_v_flag_bit = 0;

/// This is a constant used to create the x64 flags format from the ARM format.
/// NZCV * multiplier: NZCV0NZCV000NZCV
/// x64_flags format:  NZ-----C-------V
constexpr u32 to_x64_multiplier = 0x1081;

/// This is a constant used to create the ARM format from the x64 flags format.
constexpr u32 from_x64_multiplier = 0x1021'0000;

inline u32 ToX64(u32 nzcv) {
    /* Naive implementation:
    u32 x64_flags = 0;
    x64_flags |= Common::Bit<31>(cpsr) ? 1 << 15 : 0;
    x64_flags |= Common::Bit<30>(cpsr) ? 1 << 14 : 0;
    x64_flags |= Common::Bit<29>(cpsr) ? 1 << 8 : 0;
    x64_flags |= Common::Bit<28>(cpsr) ? 1 : 0;
    return x64_flags;
    */
    return ((nzcv >> 28) * to_x64_multiplier) & x64_mask;
}

inline u32 FromX64(u32 x64_flags) {
    /* Naive implementation:
    u32 nzcv = 0;
    nzcv |= Common::Bit<15>(x64_flags) ? 1 << 31 : 0;
    nzcv |= Common::Bit<14>(x64_flags) ? 1 << 30 : 0;
    nzcv |= Common::Bit<8>(x64_flags) ? 1 << 29 : 0;
    nzcv |= Common::Bit<0>(x64_flags) ? 1 << 28 : 0;
    return nzcv;
    */
    return ((x64_flags & x64_mask) * from_x64_multiplier) & arm_mask;
}

}  // namespace Dynarmic::Backend::X64::NZCV
