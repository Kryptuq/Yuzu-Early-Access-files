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

u32 ShaderIR::DecodeFfma(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    UNIMPLEMENTED_IF_MSG(instr.ffma.cc != 0, "FFMA cc not implemented");
    if (instr.ffma.tab5980_0 != 1) {
        LOG_DEBUG(HW_GPU, "FFMA tab5980_0({}) not implemented", instr.ffma.tab5980_0.Value());
    }
    if (instr.ffma.tab5980_1 != 0) {
        LOG_DEBUG(HW_GPU, "FFMA tab5980_1({}) not implemented", instr.ffma.tab5980_1.Value());
    }

    const Node op_a = GetRegister(instr.gpr8);

    auto [op_b, op_c] = [&]() -> std::tuple<Node, Node> {
        switch (opcode->get().GetId()) {
        case OpCode::Id::FFMA_CR: {
            return {GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset()),
                    GetRegister(instr.gpr39)};
        }
        case OpCode::Id::FFMA_RR:
            return {GetRegister(instr.gpr20), GetRegister(instr.gpr39)};
        case OpCode::Id::FFMA_RC: {
            return {GetRegister(instr.gpr39),
                    GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset())};
        }
        case OpCode::Id::FFMA_IMM:
            return {GetImmediate19(instr), GetRegister(instr.gpr39)};
        default:
            UNIMPLEMENTED_MSG("Unhandled FFMA instruction: {}", opcode->get().GetName());
            return {Immediate(0), Immediate(0)};
        }
    }();

    op_b = GetOperandAbsNegFloat(op_b, false, instr.ffma.negate_b);
    op_c = GetOperandAbsNegFloat(op_c, false, instr.ffma.negate_c);

    Node value = Operation(OperationCode::FFma, PRECISE, op_a, op_b, op_c);
    value = GetSaturatedFloat(value, instr.alu.saturate_d);

    SetInternalFlagsFromFloat(bb, value, instr.generates_cc);
    SetRegister(bb, instr.gpr0, value);

    return pc;
}

} // namespace VideoCommon::Shader
