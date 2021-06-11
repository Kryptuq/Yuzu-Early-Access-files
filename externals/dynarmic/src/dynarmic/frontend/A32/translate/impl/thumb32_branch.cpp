/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// BL <label>
bool TranslatorVisitor::thumb32_BL_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo) {
    const Imm<1> i1{j1 == S};
    const Imm<1> i2{j2 == S};

    if (ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    ir.PushRSB(ir.current_location.AdvancePC(4).AdvanceIT());
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 4) | 1));

    const s32 imm32 = static_cast<s32>((concatenate(S, i1, i2, hi, lo).SignExtend<u32>() << 1) + 4);
    const auto new_location = ir.current_location
                                  .AdvancePC(imm32)
                                  .AdvanceIT();
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BLX <label>
bool TranslatorVisitor::thumb32_BLX_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo) {
    const Imm<1> i1{j1 == S};
    const Imm<1> i2{j2 == S};

    if (ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    if (lo.Bit<0>()) {
        return UnpredictableInstruction();
    }

    ir.PushRSB(ir.current_location.AdvancePC(4).AdvanceIT());
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 4) | 1));

    const s32 imm32 = static_cast<s32>(concatenate(S, i1, i2, hi, lo).SignExtend<u32>() << 1);
    const auto new_location = ir.current_location
                                  .SetPC(ir.AlignPC(4) + imm32)
                                  .SetTFlag(false)
                                  .AdvanceIT();
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

bool TranslatorVisitor::thumb32_B(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo) {
    const Imm<1> i1{j1 == S};
    const Imm<1> i2{j2 == S};

    if (ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    const s32 imm32 = static_cast<s32>((concatenate(S, i1, i2, hi, lo).SignExtend<u32>() << 1) + 4);
    const auto new_location = ir.current_location
                                  .AdvancePC(imm32)
                                  .AdvanceIT();
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

bool TranslatorVisitor::thumb32_B_cond(Imm<1> S, Cond cond, Imm<6> hi, Imm<1> i1, Imm<1> i2, Imm<11> lo) {
    if (ir.current_location.IT().IsInITBlock()) {
        return UnpredictableInstruction();
    }

    // Note: i1 and i2 were not inverted from encoding and are opposite compared to the other B instructions.
    const s32 imm32 = static_cast<s32>((concatenate(S, i2, i1, hi, lo).SignExtend<u32>() << 1) + 4);
    const auto then_location = ir.current_location
                                   .AdvancePC(imm32)
                                   .AdvanceIT();
    const auto else_location = ir.current_location
                                   .AdvancePC(4)
                                   .AdvanceIT();
    ir.SetTerm(IR::Term::If{cond, IR::Term::LinkBlock{then_location}, IR::Term::LinkBlock{else_location}});
    return false;
}

}  // namespace Dynarmic::A32
