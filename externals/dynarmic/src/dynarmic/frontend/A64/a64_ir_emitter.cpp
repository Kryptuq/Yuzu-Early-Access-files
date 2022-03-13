/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/a64_ir_emitter.h"

#include "dynarmic/common/assert.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::A64 {

using Opcode = IR::Opcode;

u64 IREmitter::PC() const {
    return current_location->PC();
}

u64 IREmitter::AlignPC(size_t alignment) const {
    const u64 pc = PC();
    return static_cast<u64>(pc - pc % alignment);
}

void IREmitter::SetCheckBit(const IR::U1& value) {
    Inst(Opcode::A64SetCheckBit, value);
}

IR::U1 IREmitter::GetCFlag() {
    return Inst<IR::U1>(Opcode::A64GetCFlag);
}

IR::U32 IREmitter::GetNZCVRaw() {
    return Inst<IR::U32>(Opcode::A64GetNZCVRaw);
}

void IREmitter::SetNZCVRaw(IR::U32 value) {
    Inst(Opcode::A64SetNZCVRaw, value);
}

void IREmitter::SetNZCV(const IR::NZCV& nzcv) {
    Inst(Opcode::A64SetNZCV, nzcv);
}

void IREmitter::OrQC(const IR::U1& value) {
    Inst(Opcode::A64OrQC, value);
}

void IREmitter::CallSupervisor(u32 imm) {
    Inst(Opcode::A64CallSupervisor, Imm32(imm));
}

void IREmitter::ExceptionRaised(Exception exception) {
    Inst(Opcode::A64ExceptionRaised, Imm64(PC()), Imm64(static_cast<u64>(exception)));
}

void IREmitter::DataCacheOperationRaised(DataCacheOperation op, const IR::U64& value) {
    Inst(Opcode::A64DataCacheOperationRaised, Imm64(static_cast<u64>(op)), value);
}

void IREmitter::InstructionCacheOperationRaised(InstructionCacheOperation op, const IR::U64& value) {
    Inst(Opcode::A64InstructionCacheOperationRaised, Imm64(static_cast<u64>(op)), value);
}

void IREmitter::DataSynchronizationBarrier() {
    Inst(Opcode::A64DataSynchronizationBarrier);
}

void IREmitter::DataMemoryBarrier() {
    Inst(Opcode::A64DataMemoryBarrier);
}

void IREmitter::InstructionSynchronizationBarrier() {
    Inst(Opcode::A64InstructionSynchronizationBarrier);
}

IR::U32 IREmitter::GetCNTFRQ() {
    return Inst<IR::U32>(Opcode::A64GetCNTFRQ);
}

IR::U64 IREmitter::GetCNTPCT() {
    return Inst<IR::U64>(Opcode::A64GetCNTPCT);
}

IR::U32 IREmitter::GetCTR() {
    return Inst<IR::U32>(Opcode::A64GetCTR);
}

IR::U32 IREmitter::GetDCZID() {
    return Inst<IR::U32>(Opcode::A64GetDCZID);
}

IR::U64 IREmitter::GetTPIDR() {
    return Inst<IR::U64>(Opcode::A64GetTPIDR);
}

void IREmitter::SetTPIDR(const IR::U64& value) {
    Inst(Opcode::A64SetTPIDR, value);
}

IR::U64 IREmitter::GetTPIDRRO() {
    return Inst<IR::U64>(Opcode::A64GetTPIDRRO);
}

void IREmitter::ClearExclusive() {
    Inst(Opcode::A64ClearExclusive);
}

IR::U8 IREmitter::ReadMemory8(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U8>(Opcode::A64ReadMemory8, vaddr, IR::Value(acctype));
}

IR::U16 IREmitter::ReadMemory16(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U16>(Opcode::A64ReadMemory16, vaddr, IR::Value(acctype));
}

IR::U32 IREmitter::ReadMemory32(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U32>(Opcode::A64ReadMemory32, vaddr, IR::Value(acctype));
}

IR::U64 IREmitter::ReadMemory64(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U64>(Opcode::A64ReadMemory64, vaddr, IR::Value(acctype));
}

IR::U128 IREmitter::ReadMemory128(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U128>(Opcode::A64ReadMemory128, vaddr, IR::Value(acctype));
}

IR::U8 IREmitter::ExclusiveReadMemory8(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U8>(Opcode::A64ExclusiveReadMemory8, vaddr, IR::Value(acctype));
}

IR::U16 IREmitter::ExclusiveReadMemory16(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U16>(Opcode::A64ExclusiveReadMemory16, vaddr, IR::Value(acctype));
}

IR::U32 IREmitter::ExclusiveReadMemory32(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U32>(Opcode::A64ExclusiveReadMemory32, vaddr, IR::Value(acctype));
}

IR::U64 IREmitter::ExclusiveReadMemory64(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U64>(Opcode::A64ExclusiveReadMemory64, vaddr, IR::Value(acctype));
}

IR::U128 IREmitter::ExclusiveReadMemory128(const IR::U64& vaddr, IR::AccessType acctype) {
    return Inst<IR::U128>(Opcode::A64ExclusiveReadMemory128, vaddr, IR::Value(acctype));
}

void IREmitter::WriteMemory8(const IR::U64& vaddr, const IR::U8& value, IR::AccessType acctype) {
    Inst(Opcode::A64WriteMemory8, vaddr, value, IR::Value(acctype));
}

void IREmitter::WriteMemory16(const IR::U64& vaddr, const IR::U16& value, IR::AccessType acctype) {
    Inst(Opcode::A64WriteMemory16, vaddr, value, IR::Value(acctype));
}

void IREmitter::WriteMemory32(const IR::U64& vaddr, const IR::U32& value, IR::AccessType acctype) {
    Inst(Opcode::A64WriteMemory32, vaddr, value, IR::Value(acctype));
}

void IREmitter::WriteMemory64(const IR::U64& vaddr, const IR::U64& value, IR::AccessType acctype) {
    Inst(Opcode::A64WriteMemory64, vaddr, value, IR::Value(acctype));
}

void IREmitter::WriteMemory128(const IR::U64& vaddr, const IR::U128& value, IR::AccessType acctype) {
    Inst(Opcode::A64WriteMemory128, vaddr, value, IR::Value(acctype));
}

IR::U32 IREmitter::ExclusiveWriteMemory8(const IR::U64& vaddr, const IR::U8& value, IR::AccessType acctype) {
    return Inst<IR::U32>(Opcode::A64ExclusiveWriteMemory8, vaddr, value, IR::Value(acctype));
}

IR::U32 IREmitter::ExclusiveWriteMemory16(const IR::U64& vaddr, const IR::U16& value, IR::AccessType acctype) {
    return Inst<IR::U32>(Opcode::A64ExclusiveWriteMemory16, vaddr, value, IR::Value(acctype));
}

IR::U32 IREmitter::ExclusiveWriteMemory32(const IR::U64& vaddr, const IR::U32& value, IR::AccessType acctype) {
    return Inst<IR::U32>(Opcode::A64ExclusiveWriteMemory32, vaddr, value, IR::Value(acctype));
}

IR::U32 IREmitter::ExclusiveWriteMemory64(const IR::U64& vaddr, const IR::U64& value, IR::AccessType acctype) {
    return Inst<IR::U32>(Opcode::A64ExclusiveWriteMemory64, vaddr, value, IR::Value(acctype));
}

IR::U32 IREmitter::ExclusiveWriteMemory128(const IR::U64& vaddr, const IR::U128& value, IR::AccessType acctype) {
    return Inst<IR::U32>(Opcode::A64ExclusiveWriteMemory128, vaddr, value, IR::Value(acctype));
}

IR::U32 IREmitter::GetW(Reg reg) {
    if (reg == Reg::ZR)
        return Imm32(0);
    return Inst<IR::U32>(Opcode::A64GetW, IR::Value(reg));
}

IR::U64 IREmitter::GetX(Reg reg) {
    if (reg == Reg::ZR)
        return Imm64(0);
    return Inst<IR::U64>(Opcode::A64GetX, IR::Value(reg));
}

IR::U128 IREmitter::GetS(Vec vec) {
    return Inst<IR::U128>(Opcode::A64GetS, IR::Value(vec));
}

IR::U128 IREmitter::GetD(Vec vec) {
    return Inst<IR::U128>(Opcode::A64GetD, IR::Value(vec));
}

IR::U128 IREmitter::GetQ(Vec vec) {
    return Inst<IR::U128>(Opcode::A64GetQ, IR::Value(vec));
}

IR::U64 IREmitter::GetSP() {
    return Inst<IR::U64>(Opcode::A64GetSP);
}

IR::U32 IREmitter::GetFPCR() {
    return Inst<IR::U32>(Opcode::A64GetFPCR);
}

IR::U32 IREmitter::GetFPSR() {
    return Inst<IR::U32>(Opcode::A64GetFPSR);
}

void IREmitter::SetW(const Reg reg, const IR::U32& value) {
    if (reg == Reg::ZR)
        return;
    Inst(Opcode::A64SetW, IR::Value(reg), value);
}

void IREmitter::SetX(const Reg reg, const IR::U64& value) {
    if (reg == Reg::ZR)
        return;
    Inst(Opcode::A64SetX, IR::Value(reg), value);
}

void IREmitter::SetS(const Vec vec, const IR::U128& value) {
    Inst(Opcode::A64SetS, IR::Value(vec), value);
}

void IREmitter::SetD(const Vec vec, const IR::U128& value) {
    Inst(Opcode::A64SetD, IR::Value(vec), value);
}

void IREmitter::SetQ(const Vec vec, const IR::U128& value) {
    Inst(Opcode::A64SetQ, IR::Value(vec), value);
}

void IREmitter::SetSP(const IR::U64& value) {
    Inst(Opcode::A64SetSP, value);
}

void IREmitter::SetFPCR(const IR::U32& value) {
    Inst(Opcode::A64SetFPCR, value);
}

void IREmitter::SetFPSR(const IR::U32& value) {
    Inst(Opcode::A64SetFPSR, value);
}

void IREmitter::SetPC(const IR::U64& value) {
    Inst(Opcode::A64SetPC, value);
}

}  // namespace Dynarmic::A64
