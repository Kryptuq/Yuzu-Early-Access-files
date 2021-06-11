/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// B <label>
bool TranslatorVisitor::arm_B(Cond cond, Imm<24> imm24) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = Common::SignExtend<26, u32>(imm24.ZeroExtend() << 2) + 8;
    const auto new_location = ir.current_location.AdvancePC(imm32);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BL <label>
bool TranslatorVisitor::arm_BL(Cond cond, Imm<24> imm24) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32(ir.current_location.PC() + 4));

    const u32 imm32 = Common::SignExtend<26, u32>(imm24.ZeroExtend() << 2) + 8;
    const auto new_location = ir.current_location.AdvancePC(imm32);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BLX <label>
bool TranslatorVisitor::arm_BLX_imm(bool H, Imm<24> imm24) {
    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32(ir.current_location.PC() + 4));

    const u32 imm32 = Common::SignExtend<26, u32>((imm24.ZeroExtend() << 2)) + (H ? 2 : 0) + 8;
    const auto new_location = ir.current_location.AdvancePC(imm32).SetTFlag(true);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BLX <Rm>
bool TranslatorVisitor::arm_BLX_reg(Cond cond, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.BXWritePC(ir.GetRegister(m));
    ir.SetRegister(Reg::LR, ir.Imm32(ir.current_location.PC() + 4));
    ir.SetTerm(IR::Term::FastDispatchHint{});
    return false;
}

// BX <Rm>
bool TranslatorVisitor::arm_BX(Cond cond, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    ir.BXWritePC(ir.GetRegister(m));
    if (m == Reg::R14) {
        ir.SetTerm(IR::Term::PopRSBHint{});
    } else {
        ir.SetTerm(IR::Term::FastDispatchHint{});
    }

    return false;
}

bool TranslatorVisitor::arm_BXJ(Cond cond, Reg m) {
    // Jazelle not supported
    return arm_BX(cond, m);
}

}  // namespace Dynarmic::A32
