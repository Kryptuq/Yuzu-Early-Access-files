/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {
static bool ITBlockCheck(const A32::IREmitter& ir) {
    return ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock();
}

static bool LDMHelper(A32::IREmitter& ir, bool W, Reg n, u32 list, const IR::U32& start_address, const IR::U32& writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (Common::Bit(i, list)) {
            ir.SetRegister(static_cast<Reg>(i), ir.ReadMemory32(address));
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W && !Common::Bit(RegNumber(n), list)) {
        ir.SetRegister(n, writeback_address);
    }
    if (Common::Bit<15>(list)) {
        ir.UpdateUpperLocationDescriptor();
        ir.LoadWritePC(ir.ReadMemory32(address));
        if (n == Reg::R13) {
            ir.SetTerm(IR::Term::PopRSBHint{});
        } else {
            ir.SetTerm(IR::Term::FastDispatchHint{});
        }
        return false;
    }
    return true;
}

static bool STMHelper(A32::IREmitter& ir, bool W, Reg n, u32 list, const IR::U32& start_address, const IR::U32& writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (Common::Bit(i, list)) {
            ir.WriteMemory32(address, ir.GetRegister(static_cast<Reg>(i)));
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W) {
        ir.SetRegister(n, writeback_address);
    }
    return true;
}

bool TranslatorVisitor::thumb32_LDMDB(bool W, Reg n, Imm<16> reg_list) {
    const auto regs_imm = reg_list.ZeroExtend();
    const auto num_regs = static_cast<u32>(Common::BitCount(regs_imm));

    if (n == Reg::PC || num_regs < 2) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<15>() && reg_list.Bit<14>()) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), regs_imm)) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<13>()) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<15>() && ITBlockCheck(ir)) {
        return UnpredictableInstruction();
    }

    // Start address is the same as the writeback address.
    const IR::U32 start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(4 * num_regs));
    return LDMHelper(ir, W, n, regs_imm, start_address, start_address);
}

bool TranslatorVisitor::thumb32_LDMIA(bool W, Reg n, Imm<16> reg_list) {
    const auto regs_imm = reg_list.ZeroExtend();
    const auto num_regs = static_cast<u32>(Common::BitCount(regs_imm));

    if (n == Reg::PC || num_regs < 2) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<15>() && reg_list.Bit<14>()) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), regs_imm)) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<13>()) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<15>() && ITBlockCheck(ir)) {
        return UnpredictableInstruction();
    }

    const auto start_address = ir.GetRegister(n);
    const auto writeback_address = ir.Add(start_address, ir.Imm32(num_regs * 4));
    return LDMHelper(ir, W, n, regs_imm, start_address, writeback_address);
}

bool TranslatorVisitor::thumb32_POP(Imm<16> reg_list) {
    return thumb32_LDMIA(true, Reg::SP, reg_list);
}

bool TranslatorVisitor::thumb32_PUSH(Imm<15> reg_list) {
    return thumb32_STMDB(true, Reg::SP, reg_list);
}

bool TranslatorVisitor::thumb32_STMIA(bool W, Reg n, Imm<15> reg_list) {
    const auto regs_imm = reg_list.ZeroExtend();
    const auto num_regs = static_cast<u32>(Common::BitCount(regs_imm));

    if (n == Reg::PC || num_regs < 2) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), regs_imm)) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<13>()) {
        return UnpredictableInstruction();
    }

    const auto start_address = ir.GetRegister(n);
    const auto writeback_address = ir.Add(start_address, ir.Imm32(num_regs * 4));
    return STMHelper(ir, W, n, regs_imm, start_address, writeback_address);
}

bool TranslatorVisitor::thumb32_STMDB(bool W, Reg n, Imm<15> reg_list) {
    const auto regs_imm = reg_list.ZeroExtend();
    const auto num_regs = static_cast<u32>(Common::BitCount(regs_imm));

    if (n == Reg::PC || num_regs < 2) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), regs_imm)) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<13>()) {
        return UnpredictableInstruction();
    }

    // Start address is the same as the writeback address.
    const IR::U32 start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(4 * num_regs));
    return STMHelper(ir, W, n, regs_imm, start_address, start_address);
}

}  // namespace Dynarmic::A32
