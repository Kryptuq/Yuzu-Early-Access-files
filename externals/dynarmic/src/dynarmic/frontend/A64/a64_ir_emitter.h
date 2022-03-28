/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/A64/a64_location_descriptor.h"
#include "dynarmic/frontend/A64/a64_types.h"
#include "dynarmic/interface/A64/config.h"
#include "dynarmic/ir/ir_emitter.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::A64 {

/**
 * Convenience class to construct a basic block of the intermediate representation.
 * `block` is the resulting block.
 * The user of this class updates `current_location` as appropriate.
 */
class IREmitter : public IR::IREmitter {
public:
    explicit IREmitter(IR::Block& block)
            : IR::IREmitter(block) {}
    explicit IREmitter(IR::Block& block, LocationDescriptor descriptor)
            : IR::IREmitter(block), current_location(descriptor) {}

    std::optional<LocationDescriptor> current_location;

    u64 PC() const;
    u64 AlignPC(size_t alignment) const;

    void SetCheckBit(const IR::U1& value);
    IR::U1 GetCFlag();
    IR::U32 GetNZCVRaw();
    void SetNZCVRaw(IR::U32 value);
    void SetNZCV(const IR::NZCV& nzcv);
    void OrQC(const IR::U1& value);

    void CallSupervisor(u32 imm);
    void ExceptionRaised(Exception exception);
    void DataCacheOperationRaised(DataCacheOperation op, const IR::U64& value);
    void InstructionCacheOperationRaised(InstructionCacheOperation op, const IR::U64& value);
    void DataSynchronizationBarrier();
    void DataMemoryBarrier();
    void InstructionSynchronizationBarrier();
    IR::U32 GetCNTFRQ();
    IR::U64 GetCNTPCT();  // TODO: Ensure sub-basic-block cycle counts are updated before this.
    IR::U32 GetCTR();
    IR::U32 GetDCZID();
    IR::U64 GetTPIDR();
    IR::U64 GetTPIDRRO();
    void SetTPIDR(const IR::U64& value);

    void ClearExclusive();
    IR::U8 ReadMemory8(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U16 ReadMemory16(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U32 ReadMemory32(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U64 ReadMemory64(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U128 ReadMemory128(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U8 ExclusiveReadMemory8(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U16 ExclusiveReadMemory16(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U32 ExclusiveReadMemory32(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U64 ExclusiveReadMemory64(const IR::U64& vaddr, IR::AccType acc_type);
    IR::U128 ExclusiveReadMemory128(const IR::U64& vaddr, IR::AccType acc_type);
    void WriteMemory8(const IR::U64& vaddr, const IR::U8& value, IR::AccType acc_type);
    void WriteMemory16(const IR::U64& vaddr, const IR::U16& value, IR::AccType acc_type);
    void WriteMemory32(const IR::U64& vaddr, const IR::U32& value, IR::AccType acc_type);
    void WriteMemory64(const IR::U64& vaddr, const IR::U64& value, IR::AccType acc_type);
    void WriteMemory128(const IR::U64& vaddr, const IR::U128& value, IR::AccType acc_type);
    IR::U32 ExclusiveWriteMemory8(const IR::U64& vaddr, const IR::U8& value, IR::AccType acc_type);
    IR::U32 ExclusiveWriteMemory16(const IR::U64& vaddr, const IR::U16& value, IR::AccType acc_type);
    IR::U32 ExclusiveWriteMemory32(const IR::U64& vaddr, const IR::U32& value, IR::AccType acc_type);
    IR::U32 ExclusiveWriteMemory64(const IR::U64& vaddr, const IR::U64& value, IR::AccType acc_type);
    IR::U32 ExclusiveWriteMemory128(const IR::U64& vaddr, const IR::U128& value, IR::AccType acc_type);

    IR::U32 GetW(Reg source_reg);
    IR::U64 GetX(Reg source_reg);
    IR::U128 GetS(Vec source_vec);
    IR::U128 GetD(Vec source_vec);
    IR::U128 GetQ(Vec source_vec);
    IR::U64 GetSP();
    IR::U32 GetFPCR();
    IR::U32 GetFPSR();
    void SetW(Reg dest_reg, const IR::U32& value);
    void SetX(Reg dest_reg, const IR::U64& value);
    void SetS(Vec dest_vec, const IR::U128& value);
    void SetD(Vec dest_vec, const IR::U128& value);
    void SetQ(Vec dest_vec, const IR::U128& value);
    void SetSP(const IR::U64& value);
    void SetFPCR(const IR::U32& value);
    void SetFPSR(const IR::U32& value);
    void SetPC(const IR::U64& value);
};

}  // namespace Dynarmic::A64
