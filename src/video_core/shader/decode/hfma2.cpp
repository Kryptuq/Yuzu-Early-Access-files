// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <tuple>

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::HalfPrecision;
using Tegra::Shader::HalfType;
using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;

u32 ShaderIR::DecodeHfma2(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    if (opcode->get().GetId() == OpCode::Id::HFMA2_RR) {
        DEBUG_ASSERT(instr.hfma2.rr.precision == HalfPrecision::None);
    } else {
        DEBUG_ASSERT(instr.hfma2.precision == HalfPrecision::None);
    }

    constexpr auto identity = HalfType::H0_H1;
    bool neg_b{}, neg_c{};
    auto [saturate, type_b, op_b, type_c,
          op_c] = [&]() -> std::tuple<bool, HalfType, Node, HalfType, Node> {
        switch (opcode->get().GetId()) {
        case OpCode::Id::HFMA2_CR:
            neg_b = instr.hfma2.negate_b;
            neg_c = instr.hfma2.negate_c;
            return {instr.hfma2.saturate, HalfType::F32,
                    GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset()),
                    instr.hfma2.type_reg39, GetRegister(instr.gpr39)};
        case OpCode::Id::HFMA2_RC:
            neg_b = instr.hfma2.negate_b;
            neg_c = instr.hfma2.negate_c;
            return {instr.hfma2.saturate, instr.hfma2.type_reg39, GetRegister(instr.gpr39),
                    HalfType::F32, GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset())};
        case OpCode::Id::HFMA2_RR:
            neg_b = instr.hfma2.rr.negate_b;
            neg_c = instr.hfma2.rr.negate_c;
            return {instr.hfma2.rr.saturate, instr.hfma2.type_b, GetRegister(instr.gpr20),
                    instr.hfma2.rr.type_c, GetRegister(instr.gpr39)};
        case OpCode::Id::HFMA2_IMM_R:
            neg_c = instr.hfma2.negate_c;
            return {instr.hfma2.saturate, identity, UnpackHalfImmediate(instr, true),
                    instr.hfma2.type_reg39, GetRegister(instr.gpr39)};
        default:
            return {false, identity, Immediate(0), identity, Immediate(0)};
        }
    }();

    const Node op_a = UnpackHalfFloat(GetRegister(instr.gpr8), instr.hfma2.type_a);
    op_b = GetOperandAbsNegHalf(UnpackHalfFloat(op_b, type_b), false, neg_b);
    op_c = GetOperandAbsNegHalf(UnpackHalfFloat(op_c, type_c), false, neg_c);

    Node value = Operation(OperationCode::HFma, PRECISE, op_a, op_b, op_c);
    value = GetSaturatedHalfFloat(value, saturate);
    value = HalfMerge(GetRegister(instr.gpr0), value, instr.hfma2.merge);

    SetRegister(bb, instr.gpr0, value);

    return pc;
}

} // namespace VideoCommon::Shader
