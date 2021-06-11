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
using Tegra::Shader::Pred;

u32 ShaderIR::DecodeIntegerSetPredicate(NodeBlock& bb, u32 pc) {
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

    // We can't use the constant predicate as destination.
    ASSERT(instr.isetp.pred3 != static_cast<u64>(Pred::UnusedIndex));

    const Node second_pred = GetPredicate(instr.isetp.pred39, instr.isetp.neg_pred != 0);
    const Node predicate =
        GetPredicateComparisonInteger(instr.isetp.cond, instr.isetp.is_signed, op_a, op_b);

    // Set the primary predicate to the result of Predicate OP SecondPredicate
    const OperationCode combiner = GetPredicateCombiner(instr.isetp.op);
    const Node value = Operation(combiner, predicate, second_pred);
    SetPredicate(bb, instr.isetp.pred3, value);

    if (instr.isetp.pred0 != static_cast<u64>(Pred::UnusedIndex)) {
        // Set the secondary predicate to the result of !Predicate OP SecondPredicate, if enabled
        const Node negated_pred = Operation(OperationCode::LogicalNegate, predicate);
        SetPredicate(bb, instr.isetp.pred0, Operation(combiner, negated_pred, second_pred));
    }

    return pc;
}

} // namespace VideoCommon::Shader
