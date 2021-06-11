/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

// Register encodings used by MRS and MSR.
// Order of fields: op0, CRn, op1, op2, CRm.
enum class SystemRegisterEncoding : u32 {
    // Counter-timer Frequency register
    CNTFRQ_EL0 = 0b11'1110'011'000'0000,
    // Counter-timer Physical Count register
    CNTPCT_EL0 = 0b11'1110'011'001'0000,
    // Cache Type Register
    CTR_EL0 = 0b11'0000'011'001'0000,
    // Data Cache Zero ID register
    DCZID_EL0 = 0b11'0000'011'111'0000,
    // Floating-point Control Register
    FPCR = 0b11'0100'011'000'0100,
    // Floating-point Status Register
    FPSR = 0b11'0100'011'001'0100,
    // NZCV, Condition Flags
    NZCV = 0b11'0100'011'000'0010,
    // Read/Write Software Thread ID Register
    TPIDR_EL0 = 0b11'1101'011'010'0000,
    // Read-Only Software Thread ID Register
    TPIDRRO_EL0 = 0b11'1101'011'011'0000,
};

bool TranslatorVisitor::HINT([[maybe_unused]] Imm<4> CRm, [[maybe_unused]] Imm<3> op2) {
    return true;
}

bool TranslatorVisitor::NOP() {
    return true;
}

bool TranslatorVisitor::YIELD() {
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::Yield);
}

bool TranslatorVisitor::WFE() {
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::WaitForEvent);
}

bool TranslatorVisitor::WFI() {
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::WaitForInterrupt);
}

bool TranslatorVisitor::SEV() {
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::SendEvent);
}

bool TranslatorVisitor::SEVL() {
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::SendEventLocal);
}

bool TranslatorVisitor::CLREX(Imm<4> /*CRm*/) {
    ir.ClearExclusive();
    return true;
}

bool TranslatorVisitor::DSB(Imm<4> /*CRm*/) {
    ir.DataSynchronizationBarrier();
    return true;
}

bool TranslatorVisitor::DMB(Imm<4> /*CRm*/) {
    ir.DataMemoryBarrier();
    return true;
}

bool TranslatorVisitor::ISB(Imm<4> /*CRm*/) {
    ir.InstructionSynchronizationBarrier();
    ir.SetPC(ir.Imm64(ir.current_location->PC() + 4));
    ir.SetTerm(IR::Term::ReturnToDispatch{});
    return false;
}

bool TranslatorVisitor::MSR_reg(Imm<1> o0, Imm<3> op1, Imm<4> CRn, Imm<4> CRm, Imm<3> op2, Reg Rt) {
    const auto sys_reg = concatenate(Imm<1>{1}, o0, CRn, op1, op2, CRm).ZeroExtend<SystemRegisterEncoding>();
    switch (sys_reg) {
    case SystemRegisterEncoding::FPCR:
        ir.SetFPCR(X(32, Rt));
        ir.SetPC(ir.Imm64(ir.current_location->PC() + 4));
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    case SystemRegisterEncoding::FPSR:
        ir.SetFPSR(X(32, Rt));
        return true;
    case SystemRegisterEncoding::NZCV:
        ir.SetNZCVRaw(X(32, Rt));
        return true;
    case SystemRegisterEncoding::TPIDR_EL0:
        ir.SetTPIDR(X(64, Rt));
        return true;
    default:
        break;
    }
    return InterpretThisInstruction();
}

bool TranslatorVisitor::MRS(Imm<1> o0, Imm<3> op1, Imm<4> CRn, Imm<4> CRm, Imm<3> op2, Reg Rt) {
    const auto sys_reg = concatenate(Imm<1>{1}, o0, CRn, op1, op2, CRm).ZeroExtend<SystemRegisterEncoding>();
    switch (sys_reg) {
    case SystemRegisterEncoding::CNTFRQ_EL0:
        X(32, Rt, ir.GetCNTFRQ());
        return true;
    case SystemRegisterEncoding::CNTPCT_EL0:
        // HACK: Ensure that this is the first instruction in the block it's emitted in, so the cycle count is most up-to-date.
        if (!ir.block.empty() && !options.wall_clock_cntpct) {
            ir.block.CycleCount()--;
            ir.SetTerm(IR::Term::LinkBlock{*ir.current_location});
            return false;
        }
        X(64, Rt, ir.GetCNTPCT());
        return true;
    case SystemRegisterEncoding::CTR_EL0:
        X(32, Rt, ir.GetCTR());
        return true;
    case SystemRegisterEncoding::DCZID_EL0:
        X(32, Rt, ir.GetDCZID());
        return true;
    case SystemRegisterEncoding::FPCR:
        X(32, Rt, ir.GetFPCR());
        return true;
    case SystemRegisterEncoding::FPSR:
        X(32, Rt, ir.GetFPSR());
        return true;
    case SystemRegisterEncoding::NZCV:
        X(32, Rt, ir.GetNZCVRaw());
        return true;
    case SystemRegisterEncoding::TPIDR_EL0:
        X(64, Rt, ir.GetTPIDR());
        return true;
    case SystemRegisterEncoding::TPIDRRO_EL0:
        X(64, Rt, ir.GetTPIDRRO());
        return true;
    }
    return InterpretThisInstruction();
}

}  // namespace Dynarmic::A64
