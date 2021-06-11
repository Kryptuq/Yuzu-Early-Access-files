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
using Tegra::Shader::LogicOperation;
using Tegra::Shader::OpCode;
using Tegra::Shader::Pred;
using Tegra::Shader::PredicateResultMode;
using Tegra::Shader::Register;

u32 ShaderIR::DecodeArithmeticIntegerImmediate(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    Node op_a = GetRegister(instr.gpr8);
    Node op_b = Immediate(static_cast<s32>(instr.alu.imm20_32));

    switch (opcode->get().GetId()) {
    case OpCode::Id::IADD32I: {
        UNIMPLEMENTED_IF_MSG(instr.iadd32i.saturate, "IADD32I saturation is not implemented");

        op_a = GetOperandAbsNegInteger(std::move(op_a), false, instr.iadd32i.negate_a != 0, true);

        Node value = Operation(OperationCode::IAdd, PRECISE, std::move(op_a), std::move(op_b));

        SetInternalFlagsFromInteger(bb, value, instr.op_32.generates_cc != 0);
        SetRegister(bb, instr.gpr0, std::move(value));
        break;
    }
    case OpCode::Id::LOP32I: {
        if (instr.alu.lop32i.invert_a) {
            op_a = Operation(OperationCode::IBitwiseNot, NO_PRECISE, std::move(op_a));
        }

        if (instr.alu.lop32i.invert_b) {
            op_b = Operation(OperationCode::IBitwiseNot, NO_PRECISE, std::move(op_b));
        }

        WriteLogicOperation(bb, instr.gpr0, instr.alu.lop32i.operation, std::move(op_a),
                            std::move(op_b), PredicateResultMode::None, Pred::UnusedIndex,
                            instr.op_32.generates_cc != 0);
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled ArithmeticIntegerImmediate instruction: {}",
                          opcode->get().GetName());
    }

    return pc;
}

void ShaderIR::WriteLogicOperation(NodeBlock& bb, Register dest, LogicOperation logic_op, Node op_a,
                                   Node op_b, PredicateResultMode predicate_mode, Pred predicate,
                                   bool sets_cc) {
    Node result = [&] {
        switch (logic_op) {
        case LogicOperation::And:
            return Operation(OperationCode::IBitwiseAnd, PRECISE, std::move(op_a), std::move(op_b));
        case LogicOperation::Or:
            return Operation(OperationCode::IBitwiseOr, PRECISE, std::move(op_a), std::move(op_b));
        case LogicOperation::Xor:
            return Operation(OperationCode::IBitwiseXor, PRECISE, std::move(op_a), std::move(op_b));
        case LogicOperation::PassB:
            return op_b;
        default:
            UNIMPLEMENTED_MSG("Unimplemented logic operation={}", logic_op);
            return Immediate(0);
        }
    }();

    SetInternalFlagsFromInteger(bb, result, sets_cc);
    SetRegister(bb, dest, result);

    // Write the predicate value depending on the predicate mode.
    switch (predicate_mode) {
    case PredicateResultMode::None:
        // Do nothing.
        return;
    case PredicateResultMode::NotZero: {
        // Set the predicate to true if the result is not zero.
        Node compare = Operation(OperationCode::LogicalINotEqual, std::move(result), Immediate(0));
        SetPredicate(bb, static_cast<u64>(predicate), std::move(compare));
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unimplemented predicate result mode: {}", predicate_mode);
    }
}

} // namespace VideoCommon::Shader
