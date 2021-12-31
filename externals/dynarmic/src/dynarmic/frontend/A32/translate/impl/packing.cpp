/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

// PKHBT<c> <Rd>, <Rn>, <Rm>{, LSL #<imm>}
bool TranslatorVisitor::arm_PKHBT(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m) {
    if (n == Reg::PC || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), ShiftType::LSL, imm5, ir.Imm1(false)).result;
    const auto lower_half = ir.And(ir.GetRegister(n), ir.Imm32(0x0000FFFF));
    const auto upper_half = ir.And(shifted, ir.Imm32(0xFFFF0000));

    ir.SetRegister(d, ir.Or(lower_half, upper_half));
    return true;
}

// PKHTB<c> <Rd>, <Rn>, <Rm>{, ASR #<imm>}
bool TranslatorVisitor::arm_PKHTB(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m) {
    if (n == Reg::PC || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), ShiftType::ASR, imm5, ir.Imm1(false)).result;
    const auto lower_half = ir.And(shifted, ir.Imm32(0x0000FFFF));
    const auto upper_half = ir.And(ir.GetRegister(n), ir.Imm32(0xFFFF0000));

    ir.SetRegister(d, ir.Or(lower_half, upper_half));
    return true;
}

}  // namespace Dynarmic::A32
