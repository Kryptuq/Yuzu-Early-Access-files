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

u32 ShaderIR::DecodeFloatSetPredicate(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};

    Node op_a = GetOperandAbsNegFloat(GetRegister(instr.gpr8), instr.fsetp.abs_a != 0,
                                      instr.fsetp.neg_a != 0);
    Node op_b = [&]() {
        if (instr.is_b_imm) {
            return GetImmediate19(instr);
        } else if (instr.is_b_gpr) {
            return GetRegister(instr.gpr20);
        } else {
            return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
        }
    }();
    op_b = GetOperandAbsNegFloat(std::move(op_b), instr.fsetp.abs_b, instr.fsetp.neg_b);

    // We can't use the constant predicate as destination.
    ASSERT(instr.fsetp.pred3 != static_cast<u64>(Pred::UnusedIndex));

    const Node predicate =
        GetPredicateComparisonFloat(instr.fsetp.cond, std::move(op_a), std::move(op_b));
    const Node second_pred = GetPredicate(instr.fsetp.pred39, instr.fsetp.neg_pred != 0);

    const OperationCode combiner = GetPredicateCombiner(instr.fsetp.op);
    const Node value = Operation(combiner, predicate, second_pred);

    // Set the primary predicate to the result of Predicate OP SecondPredicate
    SetPredicate(bb, instr.fsetp.pred3, value);

    if (instr.fsetp.pred0 != static_cast<u64>(Pred::UnusedIndex)) {
        // Set the secondary predicate to the result of !Predicate OP SecondPredicate,
        // if enabled
        const Node negated_pred = Operation(OperationCode::LogicalNegate, predicate);
        const Node second_value = Operation(combiner, negated_pred, second_pred);
        SetPredicate(bb, instr.fsetp.pred0, second_value);
    }

    return pc;
}

} // namespace VideoCommon::Shader
