/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>

#include <mcl/container/intrusive_list.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/ir/value.h"

namespace Dynarmic::IR {

enum class Opcode;
enum class Type;

constexpr size_t max_arg_count = 4;

/**
 * A representation of a microinstruction. A single ARM/Thumb instruction may be
 * converted into zero or more microinstructions.
 */
class Inst final : public mcl::intrusive_list_node<Inst> {
public:
    explicit Inst(Opcode op)
            : op(op) {}

    /// Determines whether or not this instruction performs an arithmetic shift.
    bool IsArithmeticShift() const;
    /// Determines whether or not this instruction performs a logical shift.
    bool IsLogicalShift() const;
    /// Determines whether or not this instruction performs a circular shift.
    bool IsCircularShift() const;
    /// Determines whether or not this instruction performs any kind of shift.
    bool IsShift() const;

    /// Determines whether or not this instruction is a form of barrier.
    bool IsBarrier() const;

    /// Determines whether or not this instruction performs a shared memory read.
    bool IsSharedMemoryRead() const;
    /// Determines whether or not this instruction performs a shared memory write.
    bool IsSharedMemoryWrite() const;
    /// Determines whether or not this instruction performs a shared memory read or write.
    bool IsSharedMemoryReadOrWrite() const;
    /// Determines whether or not this instruction performs an atomic memory read.
    bool IsExclusiveMemoryRead() const;
    /// Determines whether or not this instruction performs an atomic memory write.
    bool IsExclusiveMemoryWrite() const;

    /// Determines whether or not this instruction performs any kind of memory read.
    bool IsMemoryRead() const;
    /// Determines whether or not this instruction performs any kind of memory write.
    bool IsMemoryWrite() const;
    /// Determines whether or not this instruction performs any kind of memory access.
    bool IsMemoryReadOrWrite() const;

    /// Determines whether or not this instruction reads from the CPSR.
    bool ReadsFromCPSR() const;
    /// Determines whether or not this instruction writes to the CPSR.
    bool WritesToCPSR() const;

    /// Determines whether or not this instruction writes to a system register.
    bool WritesToSystemRegister() const;

    /// Determines whether or not this instruction reads from a core register.
    bool ReadsFromCoreRegister() const;
    /// Determines whether or not this instruction writes to a core register.
    bool WritesToCoreRegister() const;

    /// Determines whether or not this instruction reads from the FPCR.
    bool ReadsFromFPCR() const;
    /// Determines whether or not this instruction writes to the FPCR.
    bool WritesToFPCR() const;

    /// Determines whether or not this instruction reads from the FPSR.
    bool ReadsFromFPSR() const;
    /// Determines whether or not this instruction writes to the FPSR.
    bool WritesToFPSR() const;

    /// Determines whether or not this instruction reads from the FPSR cumulative exception bits.
    bool ReadsFromFPSRCumulativeExceptionBits() const;
    /// Determines whether or not this instruction writes to the FPSR cumulative exception bits.
    bool WritesToFPSRCumulativeExceptionBits() const;
    /// Determines whether or not this instruction both reads from and writes to the FPSR cumulative exception bits.
    bool ReadsFromAndWritesToFPSRCumulativeExceptionBits() const;

    /// Determines whether or not this instruction reads from the FPSR cumulative saturation bit.
    bool ReadsFromFPSRCumulativeSaturationBit() const;
    /// Determines whether or not this instruction writes to the FPSR cumulative saturation bit.
    bool WritesToFPSRCumulativeSaturationBit() const;

    /// Determines whether or not this instruction alters memory-exclusivity.
    bool AltersExclusiveState() const;

    /// Determines whether or not this instruction accesses a coprocessor.
    bool IsCoprocessorInstruction() const;

    /// Determines whether or not this instruction causes a CPU exception.
    bool CausesCPUException() const;

    /// Determines whether or not this instruction is a SetCheckBit operation.
    bool IsSetCheckBitOperation() const;

    /// Determines whether or not this instruction may have side-effects.
    bool MayHaveSideEffects() const;

    /// Determines whether or not this instruction is a pseduo-instruction.
    /// Pseudo-instructions depend on their parent instructions for their semantics.
    bool IsAPseudoOperation() const;

    /// Determines whether or not this instruction supports the GetNZCVFromOp pseudo-operation.
    bool MayGetNZCVFromOp() const;

    /// Determines if all arguments of this instruction are immediates.
    bool AreAllArgsImmediates() const;

    size_t UseCount() const { return use_count; }
    bool HasUses() const { return use_count > 0; }

    /// Determines if there is a pseudo-operation associated with this instruction.
    bool HasAssociatedPseudoOperation() const;
    /// Gets a pseudo-operation associated with this instruction.
    Inst* GetAssociatedPseudoOperation(Opcode opcode);

    /// Get the microop this microinstruction represents.
    Opcode GetOpcode() const { return op; }
    /// Get the type this instruction returns.
    Type GetType() const;
    /// Get the number of arguments this instruction has.
    size_t NumArgs() const;

    Value GetArg(size_t index) const;
    void SetArg(size_t index, Value value);

    void Invalidate();
    void ClearArgs();

    void ReplaceUsesWith(Value replacement);

private:
    void Use(const Value& value);
    void UndoUse(const Value& value);

    Opcode op;
    size_t use_count = 0;
    std::array<Value, max_arg_count> args;

    // Linked list of pseudooperations associated with this instruction.
    Inst* next_pseudoop = nullptr;
};

}  // namespace Dynarmic::IR
