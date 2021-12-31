/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {
static IR::U32 Pack2x16To1x32(A32::IREmitter& ir, IR::U32 lo, IR::U32 hi) {
    return ir.Or(ir.And(lo, ir.Imm32(0xFFFF)), ir.LogicalShiftLeft(hi, ir.Imm8(16), ir.Imm1(0)).result);
}

static IR::U16 MostSignificantHalf(A32::IREmitter& ir, IR::U32 value) {
    return ir.LeastSignificantHalf(ir.LogicalShiftRight(value, ir.Imm8(16), ir.Imm1(0)).result);
}

using SaturationFunction = IR::ResultAndOverflow<IR::U32> (IREmitter::*)(const IR::U32&, size_t);

static bool Saturation(TranslatorVisitor& v, bool sh, Reg n, Reg d, Imm<5> shift_amount, size_t saturate_to, SaturationFunction sat_fn) {
    ASSERT_MSG(!(sh && shift_amount == 0), "Invalid decode");

    if (d == Reg::PC || n == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const auto shift = sh ? ShiftType::ASR : ShiftType::LSL;
    const auto operand = v.EmitImmShift(v.ir.GetRegister(n), shift, shift_amount, v.ir.GetCFlag());
    const auto result = (v.ir.*sat_fn)(operand.result, saturate_to);

    v.ir.SetRegister(d, result.result);
    v.ir.OrQFlag(result.overflow);
    return true;
}

static bool Saturation16(TranslatorVisitor& v, Reg n, Reg d, size_t saturate_to, SaturationFunction sat_fn) {
    if (d == Reg::PC || n == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const auto reg_n = v.ir.GetRegister(n);

    const auto lo_operand = v.ir.SignExtendHalfToWord(v.ir.LeastSignificantHalf(reg_n));
    const auto hi_operand = v.ir.SignExtendHalfToWord(MostSignificantHalf(v.ir, reg_n));
    const auto lo_result = (v.ir.*sat_fn)(lo_operand, saturate_to);
    const auto hi_result = (v.ir.*sat_fn)(hi_operand, saturate_to);

    v.ir.SetRegister(d, Pack2x16To1x32(v.ir, lo_result.result, hi_result.result));
    v.ir.OrQFlag(lo_result.overflow);
    v.ir.OrQFlag(hi_result.overflow);
    return true;
}

bool TranslatorVisitor::thumb32_ADR_t2(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = concatenate(imm1, imm3, imm8).ZeroExtend();
    const auto result = ir.AlignPC(4) - imm32;

    ir.SetRegister(d, ir.Imm32(result));
    return true;
}

bool TranslatorVisitor::thumb32_ADR_t3(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = concatenate(imm1, imm3, imm8).ZeroExtend();
    const auto result = ir.AlignPC(4) + imm32;

    ir.SetRegister(d, ir.Imm32(result));
    return true;
}

bool TranslatorVisitor::thumb32_ADD_imm_2(Imm<1> imm1, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm = concatenate(imm1, imm3, imm8).ZeroExtend();
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.AddWithCarry(reg_n, ir.Imm32(imm), ir.Imm1(0));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_BFC(Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsbit = concatenate(imm3, imm2).ZeroExtend();
    const u32 msbit = msb.ZeroExtend();

    if (msbit < lsbit) {
        return UnpredictableInstruction();
    }

    const u32 mask = ~(Common::Ones<u32>(msbit - lsbit + 1) << lsbit);
    const auto reg_d = ir.GetRegister(d);
    const auto result = ir.And(reg_d, ir.Imm32(mask));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_BFI(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsbit = concatenate(imm3, imm2).ZeroExtend();
    const u32 msbit = msb.ZeroExtend();

    if (msbit < lsbit) {
        return UnpredictableInstruction();
    }

    const u32 inclusion_mask = Common::Ones<u32>(msbit - lsbit + 1) << lsbit;
    const u32 exclusion_mask = ~inclusion_mask;
    const IR::U32 operand1 = ir.And(ir.GetRegister(d), ir.Imm32(exclusion_mask));
    const IR::U32 operand2 = ir.And(ir.LogicalShiftLeft(ir.GetRegister(n), ir.Imm8(u8(lsbit))), ir.Imm32(inclusion_mask));
    const IR::U32 result = ir.Or(operand1, operand2);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_MOVT(Imm<1> imm1, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const IR::U32 imm16 = ir.Imm32(concatenate(imm4, imm1, imm3, imm8).ZeroExtend() << 16);
    const IR::U32 operand = ir.GetRegister(d);
    const IR::U32 result = ir.Or(ir.And(operand, ir.Imm32(0x0000FFFFU)), imm16);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_MOVW_imm(Imm<1> imm1, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const IR::U32 imm = ir.Imm32(concatenate(imm4, imm1, imm3, imm8).ZeroExtend());

    ir.SetRegister(d, imm);
    return true;
}

bool TranslatorVisitor::thumb32_SBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm1) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsbit = concatenate(imm3, imm2).ZeroExtend();
    const u32 widthm1_value = widthm1.ZeroExtend();
    const u32 msb = lsbit + widthm1_value;
    if (msb >= Common::BitSize<u32>()) {
        return UnpredictableInstruction();
    }

    constexpr size_t max_width = Common::BitSize<u32>();
    const auto width = widthm1_value + 1;
    const auto left_shift_amount = static_cast<u8>(max_width - width - lsbit);
    const auto right_shift_amount = static_cast<u8>(max_width - width);
    const auto operand = ir.GetRegister(n);
    const auto tmp = ir.LogicalShiftLeft(operand, ir.Imm8(left_shift_amount));
    const auto result = ir.ArithmeticShiftRight(tmp, ir.Imm8(right_shift_amount));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SSAT(bool sh, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> sat_imm) {
    return Saturation(*this, sh, n, d, concatenate(imm3, imm2), sat_imm.ZeroExtend() + 1, &IREmitter::SignedSaturation);
}

bool TranslatorVisitor::thumb32_SSAT16(Reg n, Reg d, Imm<4> sat_imm) {
    return Saturation16(*this, n, d, sat_imm.ZeroExtend() + 1, &IREmitter::SignedSaturation);
}

bool TranslatorVisitor::thumb32_SUB_imm_2(Imm<1> imm1, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm = concatenate(imm1, imm3, imm8).ZeroExtend();
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.SubWithCarry(reg_n, ir.Imm32(imm), ir.Imm1(1));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm1) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsbit = concatenate(imm3, imm2).ZeroExtend();
    const u32 widthm1_value = widthm1.ZeroExtend();
    const u32 msb = lsbit + widthm1_value;
    if (msb >= Common::BitSize<u32>()) {
        return UnpredictableInstruction();
    }

    const auto operand = ir.GetRegister(n);
    const auto mask = ir.Imm32(Common::Ones<u32>(widthm1_value + 1));
    const auto result = ir.And(ir.LogicalShiftRight(operand, ir.Imm8(u8(lsbit))), mask);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_USAT(bool sh, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> sat_imm) {
    return Saturation(*this, sh, n, d, concatenate(imm3, imm2), sat_imm.ZeroExtend(), &IREmitter::UnsignedSaturation);
}

bool TranslatorVisitor::thumb32_USAT16(Reg n, Reg d, Imm<4> sat_imm) {
    return Saturation16(*this, n, d, sat_imm.ZeroExtend(), &IREmitter::UnsignedSaturation);
}

}  // namespace Dynarmic::A32
