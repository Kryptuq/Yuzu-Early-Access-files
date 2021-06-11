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

using Tegra::Shader::HalfType;
using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;

u32 ShaderIR::DecodeArithmeticHalf(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    bool negate_a = false;
    bool negate_b = false;
    bool absolute_a = false;
    bool absolute_b = false;

    switch (opcode->get().GetId()) {
    case OpCode::Id::HADD2_R:
        if (instr.alu_half.ftz == 0) {
            LOG_DEBUG(HW_GPU, "{} without FTZ is not implemented", opcode->get().GetName());
        }
        negate_a = ((instr.value >> 43) & 1) != 0;
        negate_b = ((instr.value >> 31) & 1) != 0;
        absolute_a = ((instr.value >> 44) & 1) != 0;
        absolute_b = ((instr.value >> 30) & 1) != 0;
        break;
    case OpCode::Id::HADD2_C:
        if (instr.alu_half.ftz == 0) {
            LOG_DEBUG(HW_GPU, "{} without FTZ is not implemented", opcode->get().GetName());
        }
        negate_a = ((instr.value >> 43) & 1) != 0;
        negate_b = ((instr.value >> 56) & 1) != 0;
        absolute_a = ((instr.value >> 44) & 1) != 0;
        absolute_b = ((instr.value >> 54) & 1) != 0;
        break;
    case OpCode::Id::HMUL2_R:
        negate_a = ((instr.value >> 43) & 1) != 0;
        absolute_a = ((instr.value >> 44) & 1) != 0;
        absolute_b = ((instr.value >> 30) & 1) != 0;
        break;
    case OpCode::Id::HMUL2_C:
        negate_b = ((instr.value >> 31) & 1) != 0;
        absolute_a = ((instr.value >> 44) & 1) != 0;
        absolute_b = ((instr.value >> 54) & 1) != 0;
        break;
    default:
        UNREACHABLE();
        break;
    }

    Node op_a = UnpackHalfFloat(GetRegister(instr.gpr8), instr.alu_half.type_a);
    op_a = GetOperandAbsNegHalf(op_a, absolute_a, negate_a);

    auto [type_b, op_b] = [this, instr, opcode]() -> std::pair<HalfType, Node> {
        switch (opcode->get().GetId()) {
        case OpCode::Id::HADD2_C:
        case OpCode::Id::HMUL2_C:
            return {HalfType::F32, GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset())};
        case OpCode::Id::HADD2_R:
        case OpCode::Id::HMUL2_R:
            return {instr.alu_half.type_b, GetRegister(instr.gpr20)};
        default:
            UNREACHABLE();
            return {HalfType::F32, Immediate(0)};
        }
    }();
    op_b = UnpackHalfFloat(op_b, type_b);
    op_b = GetOperandAbsNegHalf(op_b, absolute_b, negate_b);

    Node value = [this, opcode, op_a, op_b = op_b] {
        switch (opcode->get().GetId()) {
        case OpCode::Id::HADD2_C:
        case OpCode::Id::HADD2_R:
            return Operation(OperationCode::HAdd, PRECISE, op_a, op_b);
        case OpCode::Id::HMUL2_C:
        case OpCode::Id::HMUL2_R:
            return Operation(OperationCode::HMul, PRECISE, op_a, op_b);
        default:
            UNIMPLEMENTED_MSG("Unhandled half float instruction: {}", opcode->get().GetName());
            return Immediate(0);
        }
    }();
    value = GetSaturatedHalfFloat(value, instr.alu_half.saturate);
    value = HalfMerge(GetRegister(instr.gpr0), value, instr.alu_half.merge);

    SetRegister(bb, instr.gpr0, value);

    return pc;
}

} // namespace VideoCommon::Shader
