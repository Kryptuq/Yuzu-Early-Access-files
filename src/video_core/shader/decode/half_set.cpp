// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using std::move;
using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::PredCondition;

u32 ShaderIR::DecodeHalfSet(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    PredCondition cond{};
    bool bf = false;
    bool ftz = false;
    bool neg_a = false;
    bool abs_a = false;
    bool neg_b = false;
    bool abs_b = false;
    switch (opcode->get().GetId()) {
    case OpCode::Id::HSET2_C:
    case OpCode::Id::HSET2_IMM:
        cond = instr.hsetp2.cbuf_and_imm.cond;
        bf = instr.Bit(53);
        ftz = instr.Bit(54);
        neg_a = instr.Bit(43);
        abs_a = instr.Bit(44);
        neg_b = instr.Bit(56);
        abs_b = instr.Bit(54);
        break;
    case OpCode::Id::HSET2_R:
        cond = instr.hsetp2.reg.cond;
        bf = instr.Bit(49);
        ftz = instr.Bit(50);
        neg_a = instr.Bit(43);
        abs_a = instr.Bit(44);
        neg_b = instr.Bit(31);
        abs_b = instr.Bit(30);
        break;
    default:
        UNREACHABLE();
    }

    Node op_b = [this, instr, opcode] {
        switch (opcode->get().GetId()) {
        case OpCode::Id::HSET2_C:
            // Inform as unimplemented as this is not tested.
            UNIMPLEMENTED_MSG("HSET2_C is not implemented");
            return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
        case OpCode::Id::HSET2_R:
            return GetRegister(instr.gpr20);
        case OpCode::Id::HSET2_IMM:
            return UnpackHalfImmediate(instr, true);
        default:
            UNREACHABLE();
            return Node{};
        }
    }();

    if (!ftz) {
        LOG_DEBUG(HW_GPU, "{} without FTZ is not implemented", opcode->get().GetName());
    }

    Node op_a = UnpackHalfFloat(GetRegister(instr.gpr8), instr.hset2.type_a);
    op_a = GetOperandAbsNegHalf(op_a, abs_a, neg_a);

    switch (opcode->get().GetId()) {
    case OpCode::Id::HSET2_R:
        op_b = GetOperandAbsNegHalf(move(op_b), abs_b, neg_b);
        [[fallthrough]];
    case OpCode::Id::HSET2_C:
        op_b = UnpackHalfFloat(move(op_b), instr.hset2.type_b);
        break;
    default:
        break;
    }

    Node second_pred = GetPredicate(instr.hset2.pred39, instr.hset2.neg_pred);

    Node comparison_pair = GetPredicateComparisonHalf(cond, op_a, op_b);

    const OperationCode combiner = GetPredicateCombiner(instr.hset2.op);

    // HSET2 operates on each half float in the pack.
    std::array<Node, 2> values;
    for (u32 i = 0; i < 2; ++i) {
        const u32 raw_value = bf ? 0x3c00 : 0xffff;
        Node true_value = Immediate(raw_value << (i * 16));
        Node false_value = Immediate(0);

        Node comparison = Operation(OperationCode::LogicalPick2, comparison_pair, Immediate(i));
        Node predicate = Operation(combiner, comparison, second_pred);
        values[i] =
            Operation(OperationCode::Select, predicate, move(true_value), move(false_value));
    }

    Node value = Operation(OperationCode::UBitwiseOr, values[0], values[1]);
    SetRegister(bb, instr.gpr0, move(value));

    return pc;
}

} // namespace VideoCommon::Shader
