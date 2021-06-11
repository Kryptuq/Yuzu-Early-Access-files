/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/fp/rounding_mode.h"

namespace Dynarmic::Backend::X64 {

// Redefinition of _MM_CMPINT_* constants for use with the 'vpcmp' instruction
namespace CmpInt {
constexpr u8 Equal = 0x0;
constexpr u8 LessThan = 0x1;
constexpr u8 LessEqual = 0x2;
constexpr u8 False = 0x3;
constexpr u8 NotEqual = 0x4;
constexpr u8 NotLessThan = 0x5;
constexpr u8 GreaterEqual = 0x5;
constexpr u8 NotLessEqual = 0x6;
constexpr u8 GreaterThan = 0x6;
constexpr u8 True = 0x7;
}  // namespace CmpInt

// Used to generate ternary logic truth-tables for vpternlog
// Use these to directly refer to terms and perform binary operations upon them
// and the resulting value will be the ternary lookup table
// ex:
//  (Tern::a | ~Tern::b) & Tern::c
//      = 0b10100010
//      = 0xa2
//  vpternlog a, b, c, 0xa2
//
//  ~(Tern::a ^ Tern::b) & Tern::c
//      = 0b10000010
//      = 0x82
//  vpternlog a, b, c, 0x82
namespace Tern {
constexpr u8 a = 0b11110000;
constexpr u8 b = 0b11001100;
constexpr u8 c = 0b10101010;
}  // namespace Tern

// Opcodes for use with vfixupimm
enum class FpFixup : u8 {
    A = 0b0000,         // A
    B = 0b0001,         // B
    QNaN_B = 0b0010,    // QNaN with sign of B
    IndefNaN = 0b0011,  // Indefinite QNaN (Negative QNaN with no payload on x86)
    NegInf = 0b0100,    // -Infinity
    PosInf = 0b0101,    // +Infinity
    Inf_B = 0b0110,     // Infinity with sign of B
    NegZero = 0b0111,   // -0.0
    PosZero = 0b1000,   // +0.0
    NegOne = 0b1001,    // -1.0
    PosOne = 0b1010,    // +1.0
    Half = 0b1011,      // 0.5
    Ninety = 0b1100,    // 90.0
    HalfPi = 0b1101,    // PI/2
    PosMax = 0b1110,    // +{FLT_MAX,DBL_MAX}
    NegMax = 0b1111,    // -{FLT_MAX,DBL_MAX}
};

// Generates 32-bit LUT for vfixupimm instruction
constexpr u32 FixupLUT(FpFixup src_qnan = FpFixup::A,
                       FpFixup src_snan = FpFixup::A,
                       FpFixup src_zero = FpFixup::A,
                       FpFixup src_posone = FpFixup::A,
                       FpFixup src_neginf = FpFixup::A,
                       FpFixup src_posinf = FpFixup::A,
                       FpFixup src_pos = FpFixup::A,
                       FpFixup src_neg = FpFixup::A) {
    u32 fixup_lut = 0;
    fixup_lut = Common::ModifyBits<0, 3, u32>(fixup_lut, static_cast<u32>(src_qnan));
    fixup_lut = Common::ModifyBits<4, 7, u32>(fixup_lut, static_cast<u32>(src_snan));
    fixup_lut = Common::ModifyBits<8, 11, u32>(fixup_lut, static_cast<u32>(src_zero));
    fixup_lut = Common::ModifyBits<12, 15, u32>(fixup_lut, static_cast<u32>(src_posone));
    fixup_lut = Common::ModifyBits<16, 19, u32>(fixup_lut, static_cast<u32>(src_neginf));
    fixup_lut = Common::ModifyBits<20, 23, u32>(fixup_lut, static_cast<u32>(src_posinf));
    fixup_lut = Common::ModifyBits<24, 27, u32>(fixup_lut, static_cast<u32>(src_pos));
    fixup_lut = Common::ModifyBits<28, 31, u32>(fixup_lut, static_cast<u32>(src_neg));
    return fixup_lut;
}

constexpr std::optional<int> ConvertRoundingModeToX64Immediate(FP::RoundingMode rounding_mode) {
    switch (rounding_mode) {
    case FP::RoundingMode::ToNearest_TieEven:
        return 0b00;
    case FP::RoundingMode::TowardsPlusInfinity:
        return 0b10;
    case FP::RoundingMode::TowardsMinusInfinity:
        return 0b01;
    case FP::RoundingMode::TowardsZero:
        return 0b11;
    default:
        return std::nullopt;
    }
}

}  // namespace Dynarmic::Backend::X64
