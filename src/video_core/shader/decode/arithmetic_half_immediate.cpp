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

u32 ShaderIR::DecodeArithmeticHalfImmediate(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    if (opcode->get().GetId() == OpCode::Id::HADD2_IMM) {
        if (instr.alu_half_imm.ftz == 0) {
            LOG_DEBUG(HW_GPU, "{} without FTZ is not implemented", opcode->get().GetName());
        }
    } else {
        if (instr.alu_half_imm.precision != Tegra::Shader::HalfPrecision::FTZ) {
            LOG_DEBUG(HW_GPU, "{} without FTZ is not implemented", opcode->get().GetName());
        }
    }

    Node op_a = UnpackHalfFloat(GetRegister(instr.gpr8), instr.alu_half_imm.type_a);
    op_a = GetOperandAbsNegHalf(op_a, instr.alu_half_imm.abs_a, instr.alu_half_imm.negate_a);

    const Node op_b = UnpackHalfImmediate(instr, true);

    Node value = [&]() {
        switch (opcode->get().GetId()) {
        case OpCode::Id::HADD2_IMM:
            return Operation(OperationCode::HAdd, PRECISE, op_a, op_b);
        case OpCode::Id::HMUL2_IMM:
            return Operation(OperationCode::HMul, PRECISE, op_a, op_b);
        default:
            UNREACHABLE();
            return Immediate(0);
        }
    }();

    value = GetSaturatedHalfFloat(value, instr.alu_half_imm.saturate);
    value = HalfMerge(GetRegister(instr.gpr0), value, instr.alu_half_imm.merge);
    SetRegister(bb, instr.gpr0, value);
    return pc;
}

} // namespace VideoCommon::Shader
