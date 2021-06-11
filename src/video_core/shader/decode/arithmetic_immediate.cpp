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

u32 ShaderIR::DecodeArithmeticImmediate(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    switch (opcode->get().GetId()) {
    case OpCode::Id::MOV32_IMM: {
        SetRegister(bb, instr.gpr0, GetImmediate32(instr));
        break;
    }
    case OpCode::Id::FMUL32_IMM: {
        Node value =
            Operation(OperationCode::FMul, PRECISE, GetRegister(instr.gpr8), GetImmediate32(instr));
        value = GetSaturatedFloat(value, instr.fmul32.saturate);

        SetInternalFlagsFromFloat(bb, value, instr.op_32.generates_cc);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::FADD32I: {
        const Node op_a = GetOperandAbsNegFloat(GetRegister(instr.gpr8), instr.fadd32i.abs_a,
                                                instr.fadd32i.negate_a);
        const Node op_b = GetOperandAbsNegFloat(GetImmediate32(instr), instr.fadd32i.abs_b,
                                                instr.fadd32i.negate_b);

        const Node value = Operation(OperationCode::FAdd, PRECISE, op_a, op_b);
        SetInternalFlagsFromFloat(bb, value, instr.op_32.generates_cc);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled arithmetic immediate instruction: {}",
                          opcode->get().GetName());
    }

    return pc;
}

} // namespace VideoCommon::Shader
