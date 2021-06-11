/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// BFC<c> <Rd>, #<lsb>, #<width>
bool TranslatorVisitor::arm_BFC(Cond cond, Imm<5> msb, Reg d, Imm<5> lsb) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (msb < lsb) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 lsb_value = lsb.ZeroExtend();
    const u32 msb_value = msb.ZeroExtend();
    const u32 mask = ~(Common::Ones<u32>(msb_value - lsb_value + 1) << lsb_value);
    const IR::U32 operand = ir.GetRegister(d);
    const IR::U32 result = ir.And(operand, ir.Imm32(mask));

    ir.SetRegister(d, result);
    return true;
}

// BFI<c> <Rd>, <Rn>, #<lsb>, #<width>
bool TranslatorVisitor::arm_BFI(Cond cond, Imm<5> msb, Reg d, Imm<5> lsb, Reg n) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (msb < lsb) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 lsb_value = lsb.ZeroExtend();
    const u32 msb_value = msb.ZeroExtend();
    const u32 inclusion_mask = Common::Ones<u32>(msb_value - lsb_value + 1) << lsb_value;
    const u32 exclusion_mask = ~inclusion_mask;
    const IR::U32 operand1 = ir.And(ir.GetRegister(d), ir.Imm32(exclusion_mask));
    const IR::U32 operand2 = ir.And(ir.LogicalShiftLeft(ir.GetRegister(n), ir.Imm8(u8(lsb_value))), ir.Imm32(inclusion_mask));
    const IR::U32 result = ir.Or(operand1, operand2);

    ir.SetRegister(d, result);
    return true;
}

// CLZ<c> <Rd>, <Rm>
bool TranslatorVisitor::arm_CLZ(Cond cond, Reg d, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    ir.SetRegister(d, ir.CountLeadingZeros(ir.GetRegister(m)));
    return true;
}

// MOVT<c> <Rd>, #<imm16>
bool TranslatorVisitor::arm_MOVT(Cond cond, Imm<4> imm4, Reg d, Imm<12> imm12) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 imm16 = ir.Imm32(concatenate(imm4, imm12).ZeroExtend() << 16);
    const IR::U32 operand = ir.GetRegister(d);
    const IR::U32 result = ir.Or(ir.And(operand, ir.Imm32(0x0000FFFFU)), imm16);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::arm_MOVW(Cond cond, Imm<4> imm4, Reg d, Imm<12> imm12) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 imm = ir.Imm32(concatenate(imm4, imm12).ZeroExtend());

    ir.SetRegister(d, imm);
    return true;
}

// SBFX<c> <Rd>, <Rn>, #<lsb>, #<width>
bool TranslatorVisitor::arm_SBFX(Cond cond, Imm<5> widthm1, Reg d, Imm<5> lsb, Reg n) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsb_value = lsb.ZeroExtend();
    const u32 widthm1_value = widthm1.ZeroExtend();
    const u32 msb = lsb_value + widthm1_value;
    if (msb >= Common::BitSize<u32>()) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    constexpr size_t max_width = Common::BitSize<u32>();
    const u32 width = widthm1_value + 1;
    const u8 left_shift_amount = static_cast<u8>(max_width - width - lsb_value);
    const u8 right_shift_amount = static_cast<u8>(max_width - width);
    const IR::U32 operand = ir.GetRegister(n);
    const IR::U32 tmp = ir.LogicalShiftLeft(operand, ir.Imm8(left_shift_amount));
    const IR::U32 result = ir.ArithmeticShiftRight(tmp, ir.Imm8(right_shift_amount));

    ir.SetRegister(d, result);
    return true;
}

// SEL<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SEL(Cond cond, Reg n, Reg d, Reg m) {
    if (n == Reg::PC || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto to = ir.GetRegister(m);
    const auto from = ir.GetRegister(n);
    const auto result = ir.PackedSelect(ir.GetGEFlags(), to, from);

    ir.SetRegister(d, result);
    return true;
}

// UBFX<c> <Rd>, <Rn>, #<lsb>, #<width>
bool TranslatorVisitor::arm_UBFX(Cond cond, Imm<5> widthm1, Reg d, Imm<5> lsb, Reg n) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsb_value = lsb.ZeroExtend();
    const u32 widthm1_value = widthm1.ZeroExtend();
    const u32 msb = lsb_value + widthm1_value;
    if (msb >= Common::BitSize<u32>()) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 operand = ir.GetRegister(n);
    const IR::U32 mask = ir.Imm32(Common::Ones<u32>(widthm1_value + 1));
    const IR::U32 result = ir.And(ir.LogicalShiftRight(operand, ir.Imm8(u8(lsb_value))), mask);

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32
