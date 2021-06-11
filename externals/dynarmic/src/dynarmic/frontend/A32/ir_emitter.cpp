/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/ir_emitter.h"

#include "dynarmic/common/assert.h"
#include "dynarmic/frontend/A32/types.h"
#include "dynarmic/interface/A32/arch_version.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::A32 {

using Opcode = IR::Opcode;

size_t IREmitter::ArchVersion() const {
    switch (arch_version) {
    case ArchVersion::v3:
        return 3;
    case ArchVersion::v4:
    case ArchVersion::v4T:
        return 4;
    case ArchVersion::v5TE:
        return 5;
    case ArchVersion::v6K:
    case ArchVersion::v6T2:
        return 6;
    case ArchVersion::v7:
        return 7;
    case ArchVersion::v8:
        return 8;
    }
    UNREACHABLE();
}

u32 IREmitter::PC() const {
    const u32 offset = current_location.TFlag() ? 4 : 8;
    return current_location.PC() + offset;
}

u32 IREmitter::AlignPC(size_t alignment) const {
    const u32 pc = PC();
    return static_cast<u32>(pc - pc % alignment);
}

IR::U32 IREmitter::GetRegister(Reg reg) {
    if (reg == A32::Reg::PC) {
        return Imm32(PC());
    }
    return Inst<IR::U32>(Opcode::A32GetRegister, IR::Value(reg));
}

IR::U32U64 IREmitter::GetExtendedRegister(ExtReg reg) {
    if (A32::IsSingleExtReg(reg)) {
        return Inst<IR::U32U64>(Opcode::A32GetExtendedRegister32, IR::Value(reg));
    }

    if (A32::IsDoubleExtReg(reg)) {
        return Inst<IR::U32U64>(Opcode::A32GetExtendedRegister64, IR::Value(reg));
    }

    ASSERT_FALSE("Invalid reg.");
}

IR::U128 IREmitter::GetVector(ExtReg reg) {
    ASSERT(A32::IsDoubleExtReg(reg) || A32::IsQuadExtReg(reg));
    return Inst<IR::U128>(Opcode::A32GetVector, IR::Value(reg));
}

void IREmitter::SetRegister(const Reg reg, const IR::U32& value) {
    ASSERT(reg != A32::Reg::PC);
    Inst(Opcode::A32SetRegister, IR::Value(reg), value);
}

void IREmitter::SetExtendedRegister(const ExtReg reg, const IR::U32U64& value) {
    if (A32::IsSingleExtReg(reg)) {
        Inst(Opcode::A32SetExtendedRegister32, IR::Value(reg), value);
    } else if (A32::IsDoubleExtReg(reg)) {
        Inst(Opcode::A32SetExtendedRegister64, IR::Value(reg), value);
    } else {
        ASSERT_FALSE("Invalid reg.");
    }
}

void IREmitter::SetVector(ExtReg reg, const IR::U128& value) {
    ASSERT(A32::IsDoubleExtReg(reg) || A32::IsQuadExtReg(reg));
    Inst(Opcode::A32SetVector, IR::Value(reg), value);
}

void IREmitter::ALUWritePC(const IR::U32& value) {
    // This behaviour is ARM version-dependent.
    if (ArchVersion() >= 7 && !current_location.TFlag()) {
        BXWritePC(value);
    } else {
        BranchWritePC(value);
    }
}

void IREmitter::BranchWritePC(const IR::U32& value) {
    if (!current_location.TFlag()) {
        // Note that for ArchVersion() < 6, this is UNPREDICTABLE when value<1:0> != 0b00
        const auto new_pc = And(value, Imm32(0xFFFFFFFC));
        Inst(Opcode::A32SetRegister, IR::Value(A32::Reg::PC), new_pc);
    } else {
        const auto new_pc = And(value, Imm32(0xFFFFFFFE));
        Inst(Opcode::A32SetRegister, IR::Value(A32::Reg::PC), new_pc);
    }
}

void IREmitter::BXWritePC(const IR::U32& value) {
    Inst(Opcode::A32BXWritePC, value);
}

void IREmitter::LoadWritePC(const IR::U32& value) {
    // This behaviour is ARM version-dependent.
    if (ArchVersion() >= 5) {
        BXWritePC(value);
    } else {
        BranchWritePC(value);
    }
}

void IREmitter::UpdateUpperLocationDescriptor() {
    Inst(Opcode::A32UpdateUpperLocationDescriptor);
}

void IREmitter::CallSupervisor(const IR::U32& value) {
    Inst(Opcode::A32CallSupervisor, value);
}

void IREmitter::ExceptionRaised(const Exception exception) {
    Inst(Opcode::A32ExceptionRaised, Imm32(current_location.PC()), Imm64(static_cast<u64>(exception)));
}

IR::U32 IREmitter::GetCpsr() {
    return Inst<IR::U32>(Opcode::A32GetCpsr);
}

void IREmitter::SetCpsr(const IR::U32& value) {
    Inst(Opcode::A32SetCpsr, value);
}

void IREmitter::SetCpsrNZCV(const IR::U32& value) {
    Inst(Opcode::A32SetCpsrNZCV, value);
}

void IREmitter::SetCpsrNZCVQ(const IR::U32& value) {
    Inst(Opcode::A32SetCpsrNZCVQ, value);
}

void IREmitter::SetCheckBit(const IR::U1& value) {
    Inst(Opcode::A32SetCheckBit, value);
}

IR::U1 IREmitter::GetCFlag() {
    return Inst<IR::U1>(Opcode::A32GetCFlag);
}

void IREmitter::SetNFlag(const IR::U1& value) {
    Inst(Opcode::A32SetNFlag, value);
}

void IREmitter::SetZFlag(const IR::U1& value) {
    Inst(Opcode::A32SetZFlag, value);
}

void IREmitter::SetCFlag(const IR::U1& value) {
    Inst(Opcode::A32SetCFlag, value);
}

void IREmitter::SetVFlag(const IR::U1& value) {
    Inst(Opcode::A32SetVFlag, value);
}

void IREmitter::OrQFlag(const IR::U1& value) {
    Inst(Opcode::A32OrQFlag, value);
}

IR::U32 IREmitter::GetGEFlags() {
    return Inst<IR::U32>(Opcode::A32GetGEFlags);
}

void IREmitter::SetGEFlags(const IR::U32& value) {
    Inst(Opcode::A32SetGEFlags, value);
}

void IREmitter::SetGEFlagsCompressed(const IR::U32& value) {
    Inst(Opcode::A32SetGEFlagsCompressed, value);
}

void IREmitter::DataSynchronizationBarrier() {
    Inst(Opcode::A32DataSynchronizationBarrier);
}

void IREmitter::DataMemoryBarrier() {
    Inst(Opcode::A32DataMemoryBarrier);
}

void IREmitter::InstructionSynchronizationBarrier() {
    Inst(Opcode::A32InstructionSynchronizationBarrier);
}

IR::U32 IREmitter::GetFpscr() {
    return Inst<IR::U32>(Opcode::A32GetFpscr);
}

void IREmitter::SetFpscr(const IR::U32& new_fpscr) {
    Inst(Opcode::A32SetFpscr, new_fpscr);
}

IR::U32 IREmitter::GetFpscrNZCV() {
    return Inst<IR::U32>(Opcode::A32GetFpscrNZCV);
}

void IREmitter::SetFpscrNZCV(const IR::NZCV& new_fpscr_nzcv) {
    Inst(Opcode::A32SetFpscrNZCV, new_fpscr_nzcv);
}

void IREmitter::ClearExclusive() {
    Inst(Opcode::A32ClearExclusive);
}

IR::UAny IREmitter::ReadMemory(size_t bitsize, const IR::U32& vaddr) {
    switch (bitsize) {
    case 8:
        return ReadMemory8(vaddr);
    case 16:
        return ReadMemory16(vaddr);
    case 32:
        return ReadMemory32(vaddr);
    case 64:
        return ReadMemory64(vaddr);
    }
    ASSERT_FALSE("Invalid bitsize");
}

IR::U8 IREmitter::ReadMemory8(const IR::U32& vaddr) {
    return Inst<IR::U8>(Opcode::A32ReadMemory8, vaddr);
}

IR::U16 IREmitter::ReadMemory16(const IR::U32& vaddr) {
    const auto value = Inst<IR::U16>(Opcode::A32ReadMemory16, vaddr);
    return current_location.EFlag() ? ByteReverseHalf(value) : value;
}

IR::U32 IREmitter::ReadMemory32(const IR::U32& vaddr) {
    const auto value = Inst<IR::U32>(Opcode::A32ReadMemory32, vaddr);
    return current_location.EFlag() ? ByteReverseWord(value) : value;
}

IR::U64 IREmitter::ReadMemory64(const IR::U32& vaddr) {
    const auto value = Inst<IR::U64>(Opcode::A32ReadMemory64, vaddr);
    return current_location.EFlag() ? ByteReverseDual(value) : value;
}

IR::U8 IREmitter::ExclusiveReadMemory8(const IR::U32& vaddr) {
    return Inst<IR::U8>(Opcode::A32ExclusiveReadMemory8, vaddr);
}

IR::U16 IREmitter::ExclusiveReadMemory16(const IR::U32& vaddr) {
    const auto value = Inst<IR::U16>(Opcode::A32ExclusiveReadMemory16, vaddr);
    return current_location.EFlag() ? ByteReverseHalf(value) : value;
}

IR::U32 IREmitter::ExclusiveReadMemory32(const IR::U32& vaddr) {
    const auto value = Inst<IR::U32>(Opcode::A32ExclusiveReadMemory32, vaddr);
    return current_location.EFlag() ? ByteReverseWord(value) : value;
}

std::pair<IR::U32, IR::U32> IREmitter::ExclusiveReadMemory64(const IR::U32& vaddr) {
    const auto value = Inst<IR::U64>(Opcode::A32ExclusiveReadMemory64, vaddr);
    const auto lo = LeastSignificantWord(value);
    const auto hi = MostSignificantWord(value).result;
    if (current_location.EFlag()) {
        // DO NOT SWAP hi AND lo IN BIG ENDIAN MODE, THIS IS CORRECT BEHAVIOUR
        return std::make_pair(ByteReverseWord(lo), ByteReverseWord(hi));
    }
    return std::make_pair(lo, hi);
}

void IREmitter::WriteMemory(size_t bitsize, const IR::U32& vaddr, const IR::UAny& value) {
    switch (bitsize) {
    case 8:
        return WriteMemory8(vaddr, value);
    case 16:
        return WriteMemory16(vaddr, value);
    case 32:
        return WriteMemory32(vaddr, value);
    case 64:
        return WriteMemory64(vaddr, value);
    }
    ASSERT_FALSE("Invalid bitsize");
}

void IREmitter::WriteMemory8(const IR::U32& vaddr, const IR::U8& value) {
    Inst(Opcode::A32WriteMemory8, vaddr, value);
}

void IREmitter::WriteMemory16(const IR::U32& vaddr, const IR::U16& value) {
    if (current_location.EFlag()) {
        const auto v = ByteReverseHalf(value);
        Inst(Opcode::A32WriteMemory16, vaddr, v);
    } else {
        Inst(Opcode::A32WriteMemory16, vaddr, value);
    }
}

void IREmitter::WriteMemory32(const IR::U32& vaddr, const IR::U32& value) {
    if (current_location.EFlag()) {
        const auto v = ByteReverseWord(value);
        Inst(Opcode::A32WriteMemory32, vaddr, v);
    } else {
        Inst(Opcode::A32WriteMemory32, vaddr, value);
    }
}

void IREmitter::WriteMemory64(const IR::U32& vaddr, const IR::U64& value) {
    if (current_location.EFlag()) {
        const auto v = ByteReverseDual(value);
        Inst(Opcode::A32WriteMemory64, vaddr, v);
    } else {
        Inst(Opcode::A32WriteMemory64, vaddr, value);
    }
}

IR::U32 IREmitter::ExclusiveWriteMemory8(const IR::U32& vaddr, const IR::U8& value) {
    return Inst<IR::U32>(Opcode::A32ExclusiveWriteMemory8, vaddr, value);
}

IR::U32 IREmitter::ExclusiveWriteMemory16(const IR::U32& vaddr, const IR::U16& value) {
    if (current_location.EFlag()) {
        const auto v = ByteReverseHalf(value);
        return Inst<IR::U32>(Opcode::A32ExclusiveWriteMemory16, vaddr, v);
    } else {
        return Inst<IR::U32>(Opcode::A32ExclusiveWriteMemory16, vaddr, value);
    }
}

IR::U32 IREmitter::ExclusiveWriteMemory32(const IR::U32& vaddr, const IR::U32& value) {
    if (current_location.EFlag()) {
        const auto v = ByteReverseWord(value);
        return Inst<IR::U32>(Opcode::A32ExclusiveWriteMemory32, vaddr, v);
    } else {
        return Inst<IR::U32>(Opcode::A32ExclusiveWriteMemory32, vaddr, value);
    }
}

IR::U32 IREmitter::ExclusiveWriteMemory64(const IR::U32& vaddr, const IR::U32& value_lo, const IR::U32& value_hi) {
    if (current_location.EFlag()) {
        const auto vlo = ByteReverseWord(value_lo);
        const auto vhi = ByteReverseWord(value_hi);
        return Inst<IR::U32>(Opcode::A32ExclusiveWriteMemory64, vaddr, Pack2x32To1x64(vlo, vhi));
    } else {
        return Inst<IR::U32>(Opcode::A32ExclusiveWriteMemory64, vaddr, Pack2x32To1x64(value_lo, value_hi));
    }
}

void IREmitter::CoprocInternalOperation(size_t coproc_no, bool two, size_t opc1, CoprocReg CRd, CoprocReg CRn, CoprocReg CRm, size_t opc2) {
    ASSERT(coproc_no <= 15);
    const IR::Value::CoprocessorInfo coproc_info{static_cast<u8>(coproc_no),
                                                 static_cast<u8>(two ? 1 : 0),
                                                 static_cast<u8>(opc1),
                                                 static_cast<u8>(CRd),
                                                 static_cast<u8>(CRn),
                                                 static_cast<u8>(CRm),
                                                 static_cast<u8>(opc2)};
    Inst(Opcode::A32CoprocInternalOperation, IR::Value(coproc_info));
}

void IREmitter::CoprocSendOneWord(size_t coproc_no, bool two, size_t opc1, CoprocReg CRn, CoprocReg CRm, size_t opc2, const IR::U32& word) {
    ASSERT(coproc_no <= 15);
    const IR::Value::CoprocessorInfo coproc_info{static_cast<u8>(coproc_no),
                                                 static_cast<u8>(two ? 1 : 0),
                                                 static_cast<u8>(opc1),
                                                 static_cast<u8>(CRn),
                                                 static_cast<u8>(CRm),
                                                 static_cast<u8>(opc2)};
    Inst(Opcode::A32CoprocSendOneWord, IR::Value(coproc_info), word);
}

void IREmitter::CoprocSendTwoWords(size_t coproc_no, bool two, size_t opc, CoprocReg CRm, const IR::U32& word1, const IR::U32& word2) {
    ASSERT(coproc_no <= 15);
    const IR::Value::CoprocessorInfo coproc_info{static_cast<u8>(coproc_no),
                                                 static_cast<u8>(two ? 1 : 0),
                                                 static_cast<u8>(opc),
                                                 static_cast<u8>(CRm)};
    Inst(Opcode::A32CoprocSendTwoWords, IR::Value(coproc_info), word1, word2);
}

IR::U32 IREmitter::CoprocGetOneWord(size_t coproc_no, bool two, size_t opc1, CoprocReg CRn, CoprocReg CRm, size_t opc2) {
    ASSERT(coproc_no <= 15);
    const IR::Value::CoprocessorInfo coproc_info{static_cast<u8>(coproc_no),
                                                 static_cast<u8>(two ? 1 : 0),
                                                 static_cast<u8>(opc1),
                                                 static_cast<u8>(CRn),
                                                 static_cast<u8>(CRm),
                                                 static_cast<u8>(opc2)};
    return Inst<IR::U32>(Opcode::A32CoprocGetOneWord, IR::Value(coproc_info));
}

IR::U64 IREmitter::CoprocGetTwoWords(size_t coproc_no, bool two, size_t opc, CoprocReg CRm) {
    ASSERT(coproc_no <= 15);
    const IR::Value::CoprocessorInfo coproc_info{static_cast<u8>(coproc_no),
                                                 static_cast<u8>(two ? 1 : 0),
                                                 static_cast<u8>(opc),
                                                 static_cast<u8>(CRm)};
    return Inst<IR::U64>(Opcode::A32CoprocGetTwoWords, IR::Value(coproc_info));
}

void IREmitter::CoprocLoadWords(size_t coproc_no, bool two, bool long_transfer, CoprocReg CRd, const IR::U32& address, bool has_option, u8 option) {
    ASSERT(coproc_no <= 15);
    const IR::Value::CoprocessorInfo coproc_info{static_cast<u8>(coproc_no),
                                                 static_cast<u8>(two ? 1 : 0),
                                                 static_cast<u8>(long_transfer ? 1 : 0),
                                                 static_cast<u8>(CRd),
                                                 static_cast<u8>(has_option ? 1 : 0),
                                                 static_cast<u8>(option)};
    Inst(Opcode::A32CoprocLoadWords, IR::Value(coproc_info), address);
}

void IREmitter::CoprocStoreWords(size_t coproc_no, bool two, bool long_transfer, CoprocReg CRd, const IR::U32& address, bool has_option, u8 option) {
    ASSERT(coproc_no <= 15);
    const IR::Value::CoprocessorInfo coproc_info{static_cast<u8>(coproc_no),
                                                 static_cast<u8>(two ? 1 : 0),
                                                 static_cast<u8>(long_transfer ? 1 : 0),
                                                 static_cast<u8>(CRd),
                                                 static_cast<u8>(has_option ? 1 : 0),
                                                 static_cast<u8>(option)};
    Inst(Opcode::A32CoprocStoreWords, IR::Value(coproc_info), address);
}

}  // namespace Dynarmic::A32
