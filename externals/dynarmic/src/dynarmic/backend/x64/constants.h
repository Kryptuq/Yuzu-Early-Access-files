/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/fp/rounding_mode.h"

namespace Dynarmic::Backend::X64 {

// Redefinition of _MM_CMP_* constants for use with the 'vcmp' instruction
namespace Cmp {
constexpr u8 Equal_OQ = 0;             // Equal (Quiet, Ordered).
constexpr u8 LessThan_OS = 1;          // Less (Signaling, Ordered).
constexpr u8 LessEqual_OS = 2;         // Less/Equal (Signaling, Ordered).
constexpr u8 Unordered_Q = 3;          // Unordered (Quiet).
constexpr u8 NotEqual_UQ = 4;          // Not Equal (Quiet, Unordered).
constexpr u8 NotLessThan_US = 5;       // Not Less (Signaling, Unordered).
constexpr u8 NotLessEqual_US = 6;      // Not Less/Equal (Signaling, Unordered).
constexpr u8 Ordered_Q = 7;            // Ordered (Quiet).
constexpr u8 Equal_UQ = 8;             // Equal (Quiet, Unordered).
constexpr u8 NotGreaterEqual_US = 9;   // Not Greater/Equal (Signaling, Unordered).
constexpr u8 NotGreaterThan_US = 10;   // Not Greater (Signaling, Unordered).
constexpr u8 False_OQ = 11;            // False (Quiet, Ordered).
constexpr u8 NotEqual_OQ = 12;         // Not Equal (Quiet, Ordered).
constexpr u8 GreaterEqual_OS = 13;     // Greater/Equal (Signaling, Ordered).
constexpr u8 GreaterThan_OS = 14;      // Greater (Signaling, Ordered).
constexpr u8 True_UQ = 15;             // True (Quiet, Unordered).
constexpr u8 Equal_OS = 16;            // Equal (Signaling, Ordered).
constexpr u8 LessThan_OQ = 17;         // Less (Quiet, Ordered).
constexpr u8 LessEqual_OQ = 18;        // Less/Equal (Quiet, Ordered).
constexpr u8 Unordered_S = 19;         // Unordered (Signaling).
constexpr u8 NotEqual_US = 20;         // Not Equal (Signaling, Unordered).
constexpr u8 NotLessThan_UQ = 21;      // Not Less (Quiet, Unordered).
constexpr u8 NotLessEqual_UQ = 22;     // Not Less/Equal (Quiet, Unordered).
constexpr u8 Ordered_S = 23;           // Ordered (Signaling).
constexpr u8 Equal_US = 24;            // Equal (Signaling, Unordered).
constexpr u8 NotGreaterEqual_UQ = 25;  // Not Greater/Equal (Quiet, Unordered).
constexpr u8 NotGreaterThan_UQ = 26;   // Not Greater (Quiet, Unordered).
constexpr u8 False_OS = 27;            // False (Signaling, Ordered).
constexpr u8 NotEqual_OS = 28;         // Not Equal (Signaling, Ordered).
constexpr u8 GreaterEqual_OQ = 29;     // Greater/Equal (Quiet, Ordered).
constexpr u8 GreaterThan_OQ = 30;      // Greater (Quiet, Ordered).
constexpr u8 True_US = 31;             // True (Signaling, Unordered).
}  // namespace Cmp

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

// For use as a bitmask with vfpclass instruction
namespace FpClass {
constexpr u8 QNaN = 0b00000001;
constexpr u8 ZeroPos = 0b00000010;
constexpr u8 ZeroNeg = 0b00000100;
constexpr u8 InfPos = 0b00001000;
constexpr u8 InfNeg = 0b00010000;
constexpr u8 Denormal = 0b00100000;
constexpr u8 Negative = 0b01000000;  // Negative finite value
constexpr u8 SNaN = 0b10000000;
}  // namespace FpClass

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
    fixup_lut = mcl::bit::set_bits<0, 3, u32>(fixup_lut, static_cast<u32>(src_qnan));
    fixup_lut = mcl::bit::set_bits<4, 7, u32>(fixup_lut, static_cast<u32>(src_snan));
    fixup_lut = mcl::bit::set_bits<8, 11, u32>(fixup_lut, static_cast<u32>(src_zero));
    fixup_lut = mcl::bit::set_bits<12, 15, u32>(fixup_lut, static_cast<u32>(src_posone));
    fixup_lut = mcl::bit::set_bits<16, 19, u32>(fixup_lut, static_cast<u32>(src_neginf));
    fixup_lut = mcl::bit::set_bits<20, 23, u32>(fixup_lut, static_cast<u32>(src_posinf));
    fixup_lut = mcl::bit::set_bits<24, 27, u32>(fixup_lut, static_cast<u32>(src_pos));
    fixup_lut = mcl::bit::set_bits<28, 31, u32>(fixup_lut, static_cast<u32>(src_neg));
    return fixup_lut;
}

// Opcodes for use with vrange* instructions
enum class FpRangeSelect : u8 {
    Min = 0b00,
    Max = 0b01,
    AbsMin = 0b10,  // Smaller absolute value
    AbsMax = 0b11,  // Larger absolute value
};

enum class FpRangeSign : u8 {
    A = 0b00,         // Copy sign of operand A
    Preserve = 0b01,  // Leave sign as is
    Positive = 0b10,  // Set Positive
    Negative = 0b11,  // Set Negative
};

// Generates 8-bit immediate LUT for vrange instruction
constexpr u8 FpRangeLUT(FpRangeSelect range_select, FpRangeSign range_sign) {
    u8 range_lut = 0;
    range_lut = mcl::bit::set_bits<0, 1, u8>(range_lut, static_cast<u8>(range_select));
    range_lut = mcl::bit::set_bits<2, 3, u8>(range_lut, static_cast<u8>(range_sign));
    return range_lut;
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
