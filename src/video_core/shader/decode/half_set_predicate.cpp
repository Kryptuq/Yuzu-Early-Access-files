// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::Pred;

u32 ShaderIR::DecodeHalfSetPredicate(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    if (instr.hsetp2.ftz != 0) {
        LOG_DEBUG(HW_GPU, "{} without FTZ is not implemented", opcode->get().GetName());
    }

    Node op_a = UnpackHalfFloat(GetRegister(instr.gpr8), instr.hsetp2.type_a);
    op_a = GetOperandAbsNegHalf(op_a, instr.hsetp2.abs_a, instr.hsetp2.negate_a);

    Tegra::Shader::PredCondition cond{};
    bool h_and{};
    Node op_b{};
    switch (opcode->get().GetId()) {
    case OpCode::Id::HSETP2_C:
        cond = instr.hsetp2.cbuf_and_imm.cond;
        h_and = instr.hsetp2.cbuf_and_imm.h_and;
        op_b = GetOperandAbsNegHalf(GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset()),
                                    instr.hsetp2.cbuf.abs_b, instr.hsetp2.cbuf.negate_b);
        // F32 is hardcoded in hardware
        op_b = UnpackHalfFloat(std::move(op_b), Tegra::Shader::HalfType::F32);
        break;
    case OpCode::Id::HSETP2_IMM:
        cond = instr.hsetp2.cbuf_and_imm.cond;
        h_and = instr.hsetp2.cbuf_and_imm.h_and;
        op_b = UnpackHalfImmediate(instr, true);
        break;
    case OpCode::Id::HSETP2_R:
        cond = instr.hsetp2.reg.cond;
        h_and = instr.hsetp2.reg.h_and;
        op_b =
            GetOperandAbsNegHalf(UnpackHalfFloat(GetRegister(instr.gpr20), instr.hsetp2.reg.type_b),
                                 instr.hsetp2.reg.abs_b, instr.hsetp2.reg.negate_b);
        break;
    default:
        UNREACHABLE();
        op_b = Immediate(0);
    }

    const OperationCode combiner = GetPredicateCombiner(instr.hsetp2.op);
    const Node combined_pred = GetPredicate(instr.hsetp2.pred39, instr.hsetp2.neg_pred);

    const auto Write = [&](u64 dest, Node src) {
        SetPredicate(bb, dest, Operation(combiner, std::move(src), combined_pred));
    };

    const Node comparison = GetPredicateComparisonHalf(cond, op_a, op_b);
    const u64 first = instr.hsetp2.pred3;
    const u64 second = instr.hsetp2.pred0;
    if (h_and) {
        Node joined = Operation(OperationCode::LogicalAnd2, comparison);
        Write(first, joined);
        Write(second, Operation(OperationCode::LogicalNegate, std::move(joined)));
    } else {
        Write(first, Operation(OperationCode::LogicalPick2, comparison, Immediate(0U)));
        Write(second, Operation(OperationCode::LogicalPick2, comparison, Immediate(1U)));
    }

    return pc;
}

} // namespace VideoCommon::Shader
