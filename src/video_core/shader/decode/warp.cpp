// Copyright 2019 yuzu Emulator Project
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
using Tegra::Shader::ShuffleOperation;
using Tegra::Shader::VoteOperation;

namespace {

OperationCode GetOperationCode(VoteOperation vote_op) {
    switch (vote_op) {
    case VoteOperation::All:
        return OperationCode::VoteAll;
    case VoteOperation::Any:
        return OperationCode::VoteAny;
    case VoteOperation::Eq:
        return OperationCode::VoteEqual;
    default:
        UNREACHABLE_MSG("Invalid vote operation={}", vote_op);
        return OperationCode::VoteAll;
    }
}

} // Anonymous namespace

u32 ShaderIR::DecodeWarp(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    // Signal the backend that this shader uses warp instructions.
    uses_warps = true;

    switch (opcode->get().GetId()) {
    case OpCode::Id::VOTE: {
        const Node value = GetPredicate(instr.vote.value, instr.vote.negate_value != 0);
        const Node active = Operation(OperationCode::BallotThread, value);
        const Node vote = Operation(GetOperationCode(instr.vote.operation), value);
        SetRegister(bb, instr.gpr0, active);
        SetPredicate(bb, instr.vote.dest_pred, vote);
        break;
    }
    case OpCode::Id::SHFL: {
        Node mask = instr.shfl.is_mask_imm ? Immediate(static_cast<u32>(instr.shfl.mask_imm))
                                           : GetRegister(instr.gpr39);
        Node index = instr.shfl.is_index_imm ? Immediate(static_cast<u32>(instr.shfl.index_imm))
                                             : GetRegister(instr.gpr20);

        Node thread_id = Operation(OperationCode::ThreadId);
        Node clamp = Operation(OperationCode::IBitwiseAnd, mask, Immediate(0x1FU));
        Node seg_mask = BitfieldExtract(mask, 8, 16);

        Node neg_seg_mask = Operation(OperationCode::IBitwiseNot, seg_mask);
        Node min_thread_id = Operation(OperationCode::IBitwiseAnd, thread_id, seg_mask);
        Node max_thread_id = Operation(OperationCode::IBitwiseOr, min_thread_id,
                                       Operation(OperationCode::IBitwiseAnd, clamp, neg_seg_mask));

        Node src_thread_id = [instr, index, neg_seg_mask, min_thread_id, thread_id] {
            switch (instr.shfl.operation) {
            case ShuffleOperation::Idx:
                return Operation(OperationCode::IBitwiseOr,
                                 Operation(OperationCode::IBitwiseAnd, index, neg_seg_mask),
                                 min_thread_id);
            case ShuffleOperation::Down:
                return Operation(OperationCode::IAdd, thread_id, index);
            case ShuffleOperation::Up:
                return Operation(OperationCode::IAdd, thread_id,
                                 Operation(OperationCode::INegate, index));
            case ShuffleOperation::Bfly:
                return Operation(OperationCode::IBitwiseXor, thread_id, index);
            }
            UNREACHABLE();
            return Immediate(0U);
        }();

        Node in_bounds = [instr, src_thread_id, min_thread_id, max_thread_id] {
            if (instr.shfl.operation == ShuffleOperation::Up) {
                return Operation(OperationCode::LogicalIGreaterEqual, src_thread_id, min_thread_id);
            } else {
                return Operation(OperationCode::LogicalILessEqual, src_thread_id, max_thread_id);
            }
        }();

        SetPredicate(bb, instr.shfl.pred48, in_bounds);
        SetRegister(
            bb, instr.gpr0,
            Operation(OperationCode::ShuffleIndexed, GetRegister(instr.gpr8), src_thread_id));
        break;
    }
    case OpCode::Id::FSWZADD: {
        UNIMPLEMENTED_IF(instr.fswzadd.ndv);

        Node op_a = GetRegister(instr.gpr8);
        Node op_b = GetRegister(instr.gpr20);
        Node mask = Immediate(static_cast<u32>(instr.fswzadd.swizzle));
        SetRegister(bb, instr.gpr0, Operation(OperationCode::FSwizzleAdd, op_a, op_b, mask));
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled warp instruction: {}", opcode->get().GetName());
        break;
    }

    return pc;
}

} // namespace VideoCommon::Shader
