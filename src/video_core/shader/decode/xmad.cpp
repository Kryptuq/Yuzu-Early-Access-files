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
using Tegra::Shader::PredCondition;

u32 ShaderIR::DecodeXmad(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    UNIMPLEMENTED_IF(instr.xmad.sign_a);
    UNIMPLEMENTED_IF(instr.xmad.sign_b);
    UNIMPLEMENTED_IF_MSG(instr.generates_cc,
                         "Condition codes generation in XMAD is not implemented");

    Node op_a = GetRegister(instr.gpr8);

    // TODO(bunnei): Needs to be fixed once op_a or op_b is signed
    UNIMPLEMENTED_IF(instr.xmad.sign_a != instr.xmad.sign_b);
    const bool is_signed_a = instr.xmad.sign_a == 1;
    const bool is_signed_b = instr.xmad.sign_b == 1;
    const bool is_signed_c = is_signed_a;

    auto [is_merge, is_psl, is_high_b, mode, op_b_binding,
          op_c] = [&]() -> std::tuple<bool, bool, bool, Tegra::Shader::XmadMode, Node, Node> {
        switch (opcode->get().GetId()) {
        case OpCode::Id::XMAD_CR:
            return {instr.xmad.merge_56,
                    instr.xmad.product_shift_left_second,
                    instr.xmad.high_b,
                    instr.xmad.mode_cbf,
                    GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset()),
                    GetRegister(instr.gpr39)};
        case OpCode::Id::XMAD_RR:
            return {instr.xmad.merge_37, instr.xmad.product_shift_left, instr.xmad.high_b_rr,
                    instr.xmad.mode,     GetRegister(instr.gpr20),      GetRegister(instr.gpr39)};
        case OpCode::Id::XMAD_RC:
            return {false,
                    false,
                    instr.xmad.high_b,
                    instr.xmad.mode_cbf,
                    GetRegister(instr.gpr39),
                    GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset())};
        case OpCode::Id::XMAD_IMM:
            return {instr.xmad.merge_37,
                    instr.xmad.product_shift_left,
                    false,
                    instr.xmad.mode,
                    Immediate(static_cast<u32>(instr.xmad.imm20_16)),
                    GetRegister(instr.gpr39)};
        default:
            UNIMPLEMENTED_MSG("Unhandled XMAD instruction: {}", opcode->get().GetName());
            return {false, false, false, Tegra::Shader::XmadMode::None, Immediate(0), Immediate(0)};
        }
    }();

    op_a = SignedOperation(OperationCode::IBitfieldExtract, is_signed_a, std::move(op_a),
                           instr.xmad.high_a ? Immediate(16) : Immediate(0), Immediate(16));

    const Node original_b = op_b_binding;
    const Node op_b =
        SignedOperation(OperationCode::IBitfieldExtract, is_signed_b, std::move(op_b_binding),
                        is_high_b ? Immediate(16) : Immediate(0), Immediate(16));

    // we already check sign_a and sign_b is difference or not before so just use one in here.
    Node product = SignedOperation(OperationCode::IMul, is_signed_a, op_a, op_b);
    if (is_psl) {
        product =
            SignedOperation(OperationCode::ILogicalShiftLeft, is_signed_a, product, Immediate(16));
    }
    SetTemporary(bb, 0, product);
    product = GetTemporary(0);

    Node original_c = op_c;
    const Tegra::Shader::XmadMode set_mode = mode; // Workaround to clang compile error
    op_c = [&] {
        switch (set_mode) {
        case Tegra::Shader::XmadMode::None:
            return original_c;
        case Tegra::Shader::XmadMode::CLo:
            return BitfieldExtract(std::move(original_c), 0, 16);
        case Tegra::Shader::XmadMode::CHi:
            return BitfieldExtract(std::move(original_c), 16, 16);
        case Tegra::Shader::XmadMode::CBcc: {
            Node shifted_b = SignedOperation(OperationCode::ILogicalShiftLeft, is_signed_b,
                                             original_b, Immediate(16));
            return SignedOperation(OperationCode::IAdd, is_signed_c, std::move(original_c),
                                   std::move(shifted_b));
        }
        case Tegra::Shader::XmadMode::CSfu: {
            const Node comp_a =
                GetPredicateComparisonInteger(PredCondition::EQ, is_signed_a, op_a, Immediate(0));
            const Node comp_b =
                GetPredicateComparisonInteger(PredCondition::EQ, is_signed_b, op_b, Immediate(0));
            const Node comp = Operation(OperationCode::LogicalOr, comp_a, comp_b);

            const Node comp_minus_a = GetPredicateComparisonInteger(
                PredCondition::NE, is_signed_a,
                SignedOperation(OperationCode::IBitwiseAnd, is_signed_a, op_a,
                                Immediate(0x80000000)),
                Immediate(0));
            const Node comp_minus_b = GetPredicateComparisonInteger(
                PredCondition::NE, is_signed_b,
                SignedOperation(OperationCode::IBitwiseAnd, is_signed_b, op_b,
                                Immediate(0x80000000)),
                Immediate(0));

            Node new_c = Operation(
                OperationCode::Select, comp_minus_a,
                SignedOperation(OperationCode::IAdd, is_signed_c, original_c, Immediate(-65536)),
                original_c);
            new_c = Operation(
                OperationCode::Select, comp_minus_b,
                SignedOperation(OperationCode::IAdd, is_signed_c, new_c, Immediate(-65536)),
                std::move(new_c));

            return Operation(OperationCode::Select, comp, original_c, std::move(new_c));
        }
        default:
            UNREACHABLE();
            return Immediate(0);
        }
    }();

    SetTemporary(bb, 1, op_c);
    op_c = GetTemporary(1);

    // TODO(Rodrigo): Use an appropiate sign for this operation
    Node sum = SignedOperation(OperationCode::IAdd, is_signed_a, product, std::move(op_c));
    SetTemporary(bb, 2, sum);
    sum = GetTemporary(2);
    if (is_merge) {
        const Node a = SignedOperation(OperationCode::IBitfieldExtract, is_signed_a, std::move(sum),
                                       Immediate(0), Immediate(16));
        const Node b = SignedOperation(OperationCode::ILogicalShiftLeft, is_signed_b, original_b,
                                       Immediate(16));
        sum = SignedOperation(OperationCode::IBitwiseOr, is_signed_a, a, b);
    }

    SetInternalFlagsFromInteger(bb, sum, instr.generates_cc);
    SetRegister(bb, instr.gpr0, std::move(sum));

    return pc;
}

} // namespace VideoCommon::Shader
