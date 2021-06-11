// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using std::move;
using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::ShfType;
using Tegra::Shader::ShfXmode;

namespace {

Node IsFull(Node shift) {
    return Operation(OperationCode::LogicalIEqual, move(shift), Immediate(32));
}

Node Shift(OperationCode opcode, Node value, Node shift) {
    Node shifted = Operation(opcode, move(value), shift);
    return Operation(OperationCode::Select, IsFull(move(shift)), Immediate(0), move(shifted));
}

Node ClampShift(Node shift, s32 size = 32) {
    shift = Operation(OperationCode::IMax, move(shift), Immediate(0));
    return Operation(OperationCode::IMin, move(shift), Immediate(size));
}

Node WrapShift(Node shift, s32 size = 32) {
    return Operation(OperationCode::UBitwiseAnd, move(shift), Immediate(size - 1));
}

Node ShiftRight(Node low, Node high, Node shift, Node low_shift, ShfType type) {
    // These values are used when the shift value is less than 32
    Node less_low = Shift(OperationCode::ILogicalShiftRight, low, shift);
    Node less_high = Shift(OperationCode::ILogicalShiftLeft, high, low_shift);
    Node less = Operation(OperationCode::IBitwiseOr, move(less_high), move(less_low));

    if (type == ShfType::Bits32) {
        // On 32 bit shifts we are either full (shifting 32) or shifting less than 32 bits
        return Operation(OperationCode::Select, IsFull(move(shift)), move(high), move(less));
    }

    // And these when it's larger than or 32
    const bool is_signed = type == ShfType::S64;
    const auto opcode = SignedToUnsignedCode(OperationCode::IArithmeticShiftRight, is_signed);
    Node reduced = Operation(OperationCode::IAdd, shift, Immediate(-32));
    Node greater = Shift(opcode, high, move(reduced));

    Node is_less = Operation(OperationCode::LogicalILessThan, shift, Immediate(32));
    Node is_zero = Operation(OperationCode::LogicalIEqual, move(shift), Immediate(0));

    Node value = Operation(OperationCode::Select, move(is_less), move(less), move(greater));
    return Operation(OperationCode::Select, move(is_zero), move(high), move(value));
}

Node ShiftLeft(Node low, Node high, Node shift, Node low_shift, ShfType type) {
    // These values are used when the shift value is less than 32
    Node less_low = Operation(OperationCode::ILogicalShiftRight, low, low_shift);
    Node less_high = Operation(OperationCode::ILogicalShiftLeft, high, shift);
    Node less = Operation(OperationCode::IBitwiseOr, move(less_low), move(less_high));

    if (type == ShfType::Bits32) {
        // On 32 bit shifts we are either full (shifting 32) or shifting less than 32 bits
        return Operation(OperationCode::Select, IsFull(move(shift)), move(low), move(less));
    }

    // And these when it's larger than or 32
    Node reduced = Operation(OperationCode::IAdd, shift, Immediate(-32));
    Node greater = Shift(OperationCode::ILogicalShiftLeft, move(low), move(reduced));

    Node is_less = Operation(OperationCode::LogicalILessThan, shift, Immediate(32));
    Node is_zero = Operation(OperationCode::LogicalIEqual, move(shift), Immediate(0));

    Node value = Operation(OperationCode::Select, move(is_less), move(less), move(greater));
    return Operation(OperationCode::Select, move(is_zero), move(high), move(value));
}

} // Anonymous namespace

u32 ShaderIR::DecodeShift(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    Node op_a = GetRegister(instr.gpr8);
    Node op_b = [this, instr] {
        if (instr.is_b_imm) {
            return Immediate(instr.alu.GetSignedImm20_20());
        } else if (instr.is_b_gpr) {
            return GetRegister(instr.gpr20);
        } else {
            return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
        }
    }();

    switch (const auto opid = opcode->get().GetId(); opid) {
    case OpCode::Id::SHR_C:
    case OpCode::Id::SHR_R:
    case OpCode::Id::SHR_IMM: {
        op_b = instr.shr.wrap ? WrapShift(move(op_b)) : ClampShift(move(op_b));

        Node value = SignedOperation(OperationCode::IArithmeticShiftRight, instr.shift.is_signed,
                                     move(op_a), move(op_b));
        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
        SetRegister(bb, instr.gpr0, move(value));
        break;
    }
    case OpCode::Id::SHL_C:
    case OpCode::Id::SHL_R:
    case OpCode::Id::SHL_IMM: {
        Node value = Operation(OperationCode::ILogicalShiftLeft, op_a, op_b);
        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
        SetRegister(bb, instr.gpr0, move(value));
        break;
    }
    case OpCode::Id::SHF_RIGHT_R:
    case OpCode::Id::SHF_RIGHT_IMM:
    case OpCode::Id::SHF_LEFT_R:
    case OpCode::Id::SHF_LEFT_IMM: {
        UNIMPLEMENTED_IF(instr.generates_cc);
        UNIMPLEMENTED_IF_MSG(instr.shf.xmode != ShfXmode::None, "xmode={}",
                             instr.shf.xmode.Value());

        if (instr.is_b_imm) {
            op_b = Immediate(static_cast<u32>(instr.shf.immediate));
        }
        const s32 size = instr.shf.type == ShfType::Bits32 ? 32 : 64;
        Node shift = instr.shf.wrap ? WrapShift(move(op_b), size) : ClampShift(move(op_b), size);

        Node negated_shift = Operation(OperationCode::INegate, shift);
        Node low_shift = Operation(OperationCode::IAdd, move(negated_shift), Immediate(32));

        const bool is_right = opid == OpCode::Id::SHF_RIGHT_R || opid == OpCode::Id::SHF_RIGHT_IMM;
        Node value = (is_right ? ShiftRight : ShiftLeft)(
            move(op_a), GetRegister(instr.gpr39), move(shift), move(low_shift), instr.shf.type);

        SetRegister(bb, instr.gpr0, move(value));
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled shift instruction: {}", opcode->get().GetName());
    }

    return pc;
}

} // namespace VideoCommon::Shader
