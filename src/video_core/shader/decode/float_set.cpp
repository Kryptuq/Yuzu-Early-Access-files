// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;

u32 ShaderIR::DecodeFloatSet(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};

    const Node op_a = GetOperandAbsNegFloat(GetRegister(instr.gpr8), instr.fset.abs_a != 0,
                                            instr.fset.neg_a != 0);

    Node op_b = [&]() {
        if (instr.is_b_imm) {
            return GetImmediate19(instr);
        } else if (instr.is_b_gpr) {
            return GetRegister(instr.gpr20);
        } else {
            return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
        }
    }();

    op_b = GetOperandAbsNegFloat(op_b, instr.fset.abs_b != 0, instr.fset.neg_b != 0);

    // The fset instruction sets a register to 1.0 or -1 (depending on the bf bit) if the
    // condition is true, and to 0 otherwise.
    const Node second_pred = GetPredicate(instr.fset.pred39, instr.fset.neg_pred != 0);

    const OperationCode combiner = GetPredicateCombiner(instr.fset.op);
    const Node first_pred = GetPredicateComparisonFloat(instr.fset.cond, op_a, op_b);

    const Node predicate = Operation(combiner, first_pred, second_pred);

    const Node true_value = instr.fset.bf ? Immediate(1.0f) : Immediate(-1);
    const Node false_value = instr.fset.bf ? Immediate(0.0f) : Immediate(0);
    const Node value =
        Operation(OperationCode::Select, PRECISE, predicate, true_value, false_value);

    if (instr.fset.bf) {
        SetInternalFlagsFromFloat(bb, value, instr.generates_cc);
    } else {
        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
    }
    SetRegister(bb, instr.gpr0, value);

    return pc;
}

} // namespace VideoCommon::Shader
