/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// RBIT<c> <Rd>, <Rm>
bool TranslatorVisitor::arm_RBIT(Cond cond, Reg d, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 swapped = ir.ByteReverseWord(ir.GetRegister(m));

    // ((x & 0xF0F0F0F0) >> 4) | ((x & 0x0F0F0F0F) << 4)
    const IR::U32 first_lsr = ir.LogicalShiftRight(ir.And(swapped, ir.Imm32(0xF0F0F0F0)), ir.Imm8(4));
    const IR::U32 first_lsl = ir.LogicalShiftLeft(ir.And(swapped, ir.Imm32(0x0F0F0F0F)), ir.Imm8(4));
    const IR::U32 corrected = ir.Or(first_lsl, first_lsr);

    // ((x & 0x88888888) >> 3) | ((x & 0x44444444) >> 1) |
    // ((x & 0x22222222) << 1) | ((x & 0x11111111) << 3)
    const IR::U32 second_lsr = ir.LogicalShiftRight(ir.And(corrected, ir.Imm32(0x88888888)), ir.Imm8(3));
    const IR::U32 third_lsr = ir.LogicalShiftRight(ir.And(corrected, ir.Imm32(0x44444444)), ir.Imm8(1));
    const IR::U32 second_lsl = ir.LogicalShiftLeft(ir.And(corrected, ir.Imm32(0x22222222)), ir.Imm8(1));
    const IR::U32 third_lsl = ir.LogicalShiftLeft(ir.And(corrected, ir.Imm32(0x11111111)), ir.Imm8(3));

    const IR::U32 result = ir.Or(ir.Or(ir.Or(second_lsr, third_lsr), second_lsl), third_lsl);

    ir.SetRegister(d, result);
    return true;
}

// REV<c> <Rd>, <Rm>
bool TranslatorVisitor::arm_REV(Cond cond, Reg d, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.ByteReverseWord(ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// REV16<c> <Rd>, <Rm>
bool TranslatorVisitor::arm_REV16(Cond cond, Reg d, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto reg_m = ir.GetRegister(m);
    const auto lo = ir.And(ir.LogicalShiftRight(reg_m, ir.Imm8(8), ir.Imm1(0)).result, ir.Imm32(0x00FF00FF));
    const auto hi = ir.And(ir.LogicalShiftLeft(reg_m, ir.Imm8(8), ir.Imm1(0)).result, ir.Imm32(0xFF00FF00));
    const auto result = ir.Or(lo, hi);

    ir.SetRegister(d, result);
    return true;
}

// REVSH<c> <Rd>, <Rm>
bool TranslatorVisitor::arm_REVSH(Cond cond, Reg d, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rev_half = ir.ByteReverseHalf(ir.LeastSignificantHalf(ir.GetRegister(m)));
    ir.SetRegister(d, ir.SignExtendHalfToWord(rev_half));
    return true;
}

}  // namespace Dynarmic::A32
