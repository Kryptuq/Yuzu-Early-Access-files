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

u32 ShaderIR::DecodeBfe(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    Node op_a = GetRegister(instr.gpr8);
    Node op_b = [&] {
        switch (opcode->get().GetId()) {
        case OpCode::Id::BFE_R:
            return GetRegister(instr.gpr20);
        case OpCode::Id::BFE_C:
            return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
        case OpCode::Id::BFE_IMM:
            return Immediate(instr.alu.GetSignedImm20_20());
        default:
            UNREACHABLE();
            return Immediate(0);
        }
    }();

    UNIMPLEMENTED_IF_MSG(instr.bfe.rd_cc, "Condition codes in BFE is not implemented");

    const bool is_signed = instr.bfe.is_signed;

    // using reverse parallel method in
    // https://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
    // note for later if possible to implement faster method.
    if (instr.bfe.brev) {
        const auto swap = [&](u32 s, u32 mask) {
            Node v1 =
                SignedOperation(OperationCode::ILogicalShiftRight, is_signed, op_a, Immediate(s));
            if (mask != 0) {
                v1 = SignedOperation(OperationCode::IBitwiseAnd, is_signed, std::move(v1),
                                     Immediate(mask));
            }
            Node v2 = op_a;
            if (mask != 0) {
                v2 = SignedOperation(OperationCode::IBitwiseAnd, is_signed, std::move(v2),
                                     Immediate(mask));
            }
            v2 = SignedOperation(OperationCode::ILogicalShiftLeft, is_signed, std::move(v2),
                                 Immediate(s));
            return SignedOperation(OperationCode::IBitwiseOr, is_signed, std::move(v1),
                                   std::move(v2));
        };
        op_a = swap(1, 0x55555555U);
        op_a = swap(2, 0x33333333U);
        op_a = swap(4, 0x0F0F0F0FU);
        op_a = swap(8, 0x00FF00FFU);
        op_a = swap(16, 0);
    }

    const auto offset = SignedOperation(OperationCode::IBitfieldExtract, is_signed, op_b,
                                        Immediate(0), Immediate(8));
    const auto bits = SignedOperation(OperationCode::IBitfieldExtract, is_signed, op_b,
                                      Immediate(8), Immediate(8));
    auto result = SignedOperation(OperationCode::IBitfieldExtract, is_signed, op_a, offset, bits);
    SetRegister(bb, instr.gpr0, std::move(result));

    return pc;
}

} // namespace VideoCommon::Shader
