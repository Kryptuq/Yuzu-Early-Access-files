/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::thumb32_CLZ(Reg n, Reg d, Reg m) {
    if (m != n || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto result = ir.CountLeadingZeros(reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_QADD(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.SignedSaturatedAdd(reg_m, reg_n);

    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

bool TranslatorVisitor::thumb32_QDADD(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto doubled_n = ir.SignedSaturatedAdd(reg_n, reg_n);
    ir.OrQFlag(doubled_n.overflow);

    const auto result = ir.SignedSaturatedAdd(reg_m, doubled_n.result);
    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

bool TranslatorVisitor::thumb32_QDSUB(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto doubled_n = ir.SignedSaturatedAdd(reg_n, reg_n);
    ir.OrQFlag(doubled_n.overflow);

    const auto result = ir.SignedSaturatedSub(reg_m, doubled_n.result);
    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

bool TranslatorVisitor::thumb32_QSUB(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.SignedSaturatedSub(reg_m, reg_n);

    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

bool TranslatorVisitor::thumb32_RBIT(Reg n, Reg d, Reg m) {
    if (m != n || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_REV(Reg n, Reg d, Reg m) {
    if (m != n || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto result = ir.ByteReverseWord(ir.GetRegister(m));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_REV16(Reg n, Reg d, Reg m) {
    if (m != n || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto lo = ir.And(ir.LogicalShiftRight(reg_m, ir.Imm8(8), ir.Imm1(0)).result, ir.Imm32(0x00FF00FF));
    const auto hi = ir.And(ir.LogicalShiftLeft(reg_m, ir.Imm8(8), ir.Imm1(0)).result, ir.Imm32(0xFF00FF00));
    const auto result = ir.Or(lo, hi);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_REVSH(Reg n, Reg d, Reg m) {
    if (m != n || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto rev_half = ir.ByteReverseHalf(ir.LeastSignificantHalf(reg_m));

    ir.SetRegister(d, ir.SignExtendHalfToWord(rev_half));
    return true;
}

bool TranslatorVisitor::thumb32_SEL(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSelect(ir.GetGEFlags(), reg_m, reg_n);

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32
