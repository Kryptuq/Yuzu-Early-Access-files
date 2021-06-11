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

u32 ShaderIR::DecodeBfi(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    const auto [packed_shift, base] = [&]() -> std::pair<Node, Node> {
        switch (opcode->get().GetId()) {
        case OpCode::Id::BFI_RC:
            return {GetRegister(instr.gpr39),
                    GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset())};
        case OpCode::Id::BFI_IMM_R:
            return {Immediate(instr.alu.GetSignedImm20_20()), GetRegister(instr.gpr39)};
        default:
            UNREACHABLE();
            return {Immediate(0), Immediate(0)};
        }
    }();
    const Node insert = GetRegister(instr.gpr8);
    const Node offset = BitfieldExtract(packed_shift, 0, 8);
    const Node bits = BitfieldExtract(packed_shift, 8, 8);

    const Node value =
        Operation(OperationCode::UBitfieldInsert, PRECISE, base, insert, offset, bits);

    SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
    SetRegister(bb, instr.gpr0, value);

    return pc;
}

} // namespace VideoCommon::Shader
