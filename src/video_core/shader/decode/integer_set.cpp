// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;

u32 ShaderIR::DecodeIntegerSet(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};

    const Node op_a = GetRegister(instr.gpr8);
    const Node op_b = [&]() {
        if (instr.is_b_imm) {
            return Immediate(instr.alu.GetSignedImm20_20());
        } else if (instr.is_b_gpr) {
            return GetRegister(instr.gpr20);
        } else {
            return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
        }
    }();

    // The iset instruction sets a register to 1.0 or -1 (depending on the bf bit) if the condition
    // is true, and to 0 otherwise.
    const Node second_pred = GetPredicate(instr.iset.pred39, instr.iset.neg_pred != 0);
    const Node first_pred =
        GetPredicateComparisonInteger(instr.iset.cond, instr.iset.is_signed, op_a, op_b);

    const OperationCode combiner = GetPredicateCombiner(instr.iset.op);

    const Node predicate = Operation(combiner, first_pred, second_pred);

    const Node true_value = instr.iset.bf ? Immediate(1.0f) : Immediate(-1);
    const Node false_value = instr.iset.bf ? Immediate(0.0f) : Immediate(0);
    const Node value =
        Operation(OperationCode::Select, PRECISE, predicate, true_value, false_value);

    SetRegister(bb, instr.gpr0, value);

    return pc;
}

} // namespace VideoCommon::Shader
