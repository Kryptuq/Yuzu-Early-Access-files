/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {
static bool ITBlockCheck(const A32::IREmitter& ir) {
    return ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock();
}

bool TranslatorVisitor::thumb32_LDR_lit(bool U, Reg t, Imm<12> imm12) {
    if (t == Reg::PC && ITBlockCheck(ir)) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const u32 base = ir.AlignPC(4);
    const u32 address = U ? base + imm32 : base - imm32;
    const auto data = ir.ReadMemory32(ir.Imm32(address));

    if (t == Reg::PC) {
        ir.UpdateUpperLocationDescriptor();
        ir.LoadWritePC(data);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

bool TranslatorVisitor::thumb32_LDR_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!P && !W) {
        return UndefinedInstruction();
    }
    if (W && n == t) {
        return UnpredictableInstruction();
    }
    if (t == Reg::PC && ITBlockCheck(ir)) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend();
    const IR::U32 reg_n = ir.GetRegister(n);
    const IR::U32 offset_address = U ? ir.Add(reg_n, ir.Imm32(imm32))
                                     : ir.Sub(reg_n, ir.Imm32(imm32));
    const IR::U32 address = P ? offset_address
                              : reg_n;
    const IR::U32 data = ir.ReadMemory32(address);

    if (W) {
        ir.SetRegister(n, offset_address);
    }

    if (t == Reg::PC) {
        ir.UpdateUpperLocationDescriptor();
        ir.LoadWritePC(data);

        if (!P && W && n == Reg::R13) {
            ir.SetTerm(IR::Term::PopRSBHint{});
        } else {
            ir.SetTerm(IR::Term::FastDispatchHint{});
        }

        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

bool TranslatorVisitor::thumb32_LDR_imm12(Reg n, Reg t, Imm<12> imm12) {
    if (t == Reg::PC && ITBlockCheck(ir)) {
        return UnpredictableInstruction();
    }

    const auto imm32 = imm12.ZeroExtend();
    const auto reg_n = ir.GetRegister(n);
    const auto address = ir.Add(reg_n, ir.Imm32(imm32));
    const auto data = ir.ReadMemory32(address);

    if (t == Reg::PC) {
        ir.UpdateUpperLocationDescriptor();
        ir.LoadWritePC(data);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

bool TranslatorVisitor::thumb32_LDR_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (t == Reg::PC && ITBlockCheck(ir)) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto offset = ir.LogicalShiftLeft(reg_m, ir.Imm8(imm2.ZeroExtend<u8>()));
    const auto address = ir.Add(reg_n, offset);
    const auto data = ir.ReadMemory32(address);

    if (t == Reg::PC) {
        ir.UpdateUpperLocationDescriptor();
        ir.LoadWritePC(data);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

bool TranslatorVisitor::thumb32_LDRT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution levels.

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat it as a normal LDR, given we don't support
    // execution levels other than EL0 currently.
    return thumb32_LDR_imm8(n, t, true, true, false, imm8);
}

}  // namespace Dynarmic::A32
