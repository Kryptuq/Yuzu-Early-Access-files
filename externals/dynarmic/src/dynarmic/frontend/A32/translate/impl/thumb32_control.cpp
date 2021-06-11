/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::thumb32_BXJ(Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    return thumb16_BX(m);
}

bool TranslatorVisitor::thumb32_CLREX() {
    ir.ClearExclusive();
    return true;
}

bool TranslatorVisitor::thumb32_DMB(Imm<4> /*option*/) {
    ir.DataMemoryBarrier();
    return true;
}

bool TranslatorVisitor::thumb32_DSB(Imm<4> /*option*/) {
    ir.DataSynchronizationBarrier();
    return true;
}

bool TranslatorVisitor::thumb32_ISB(Imm<4> /*option*/) {
    ir.InstructionSynchronizationBarrier();
    ir.UpdateUpperLocationDescriptor();
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
    ir.SetTerm(IR::Term::ReturnToDispatch{});
    return false;
}

bool TranslatorVisitor::thumb32_NOP() {
    return thumb16_NOP();
}

bool TranslatorVisitor::thumb32_SEV() {
    return thumb16_SEV();
}

bool TranslatorVisitor::thumb32_SEVL() {
    return thumb16_SEVL();
}

bool TranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

bool TranslatorVisitor::thumb32_WFE() {
    return thumb16_WFE();
}

bool TranslatorVisitor::thumb32_WFI() {
    return thumb16_WFI();
}

bool TranslatorVisitor::thumb32_YIELD() {
    return thumb16_YIELD();
}

bool TranslatorVisitor::thumb32_MSR_reg(bool write_spsr, Reg n, Imm<4> mask) {
    if (mask == 0) {
        return UnpredictableInstruction();
    }

    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (write_spsr) {
        return UndefinedInstruction();
    }

    const bool write_nzcvq = mask.Bit<3>();
    const bool write_g = mask.Bit<2>();
    const bool write_e = mask.Bit<1>();
    const auto value = ir.GetRegister(n);

    if (!write_e) {
        if (write_nzcvq) {
            ir.SetCpsrNZCVQ(ir.And(value, ir.Imm32(0xF8000000)));
        }

        if (write_g) {
            ir.SetGEFlagsCompressed(ir.And(value, ir.Imm32(0x000F0000)));
        }
    } else {
        ir.UpdateUpperLocationDescriptor();

        const u32 cpsr_mask = (write_nzcvq ? 0xF8000000 : 0) | (write_g ? 0x000F0000 : 0) | 0x00000200;
        const auto old_cpsr = ir.And(ir.GetCpsr(), ir.Imm32(~cpsr_mask));
        const auto new_cpsr = ir.And(value, ir.Imm32(cpsr_mask));
        ir.SetCpsr(ir.Or(old_cpsr, new_cpsr));
        ir.PushRSB(ir.current_location.AdvancePC(4).AdvanceIT());
        ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
        ir.SetTerm(IR::Term::CheckHalt{IR::Term::PopRSBHint{}});
        return false;
    }

    return true;
}

bool TranslatorVisitor::thumb32_MRS_reg(bool read_spsr, Reg d) {
    if (d == Reg::R15) {
        return UnpredictableInstruction();
    }

    // TODO: Revisit when implementing more than user mode.

    if (read_spsr) {
        return UndefinedInstruction();
    }

    ir.SetRegister(d, ir.GetCpsr());
    return true;
}

}  // namespace Dynarmic::A32
