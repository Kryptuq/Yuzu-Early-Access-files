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
using Tegra::Shader::Pred;
using Tegra::Shader::VideoType;
using Tegra::Shader::VmadShr;
using Tegra::Shader::VmnmxOperation;
using Tegra::Shader::VmnmxType;

u32 ShaderIR::DecodeVideo(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    if (opcode->get().GetId() == OpCode::Id::VMNMX) {
        DecodeVMNMX(bb, instr);
        return pc;
    }

    const Node op_a =
        GetVideoOperand(GetRegister(instr.gpr8), instr.video.is_byte_chunk_a, instr.video.signed_a,
                        instr.video.type_a, instr.video.byte_height_a);
    const Node op_b = [this, instr] {
        if (instr.video.use_register_b) {
            return GetVideoOperand(GetRegister(instr.gpr20), instr.video.is_byte_chunk_b,
                                   instr.video.signed_b, instr.video.type_b,
                                   instr.video.byte_height_b);
        }
        if (instr.video.signed_b) {
            const auto imm = static_cast<s16>(instr.alu.GetImm20_16());
            return Immediate(static_cast<u32>(imm));
        } else {
            return Immediate(instr.alu.GetImm20_16());
        }
    }();

    switch (opcode->get().GetId()) {
    case OpCode::Id::VMAD: {
        const bool result_signed = instr.video.signed_a == 1 || instr.video.signed_b == 1;
        const Node op_c = GetRegister(instr.gpr39);

        Node value = SignedOperation(OperationCode::IMul, result_signed, NO_PRECISE, op_a, op_b);
        value = SignedOperation(OperationCode::IAdd, result_signed, NO_PRECISE, value, op_c);

        if (instr.vmad.shr == VmadShr::Shr7 || instr.vmad.shr == VmadShr::Shr15) {
            const Node shift = Immediate(instr.vmad.shr == VmadShr::Shr7 ? 7 : 15);
            value =
                SignedOperation(OperationCode::IArithmeticShiftRight, result_signed, value, shift);
        }

        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::VSETP: {
        // We can't use the constant predicate as destination.
        ASSERT(instr.vsetp.pred3 != static_cast<u64>(Pred::UnusedIndex));

        const bool sign = instr.video.signed_a == 1 || instr.video.signed_b == 1;
        const Node first_pred = GetPredicateComparisonInteger(instr.vsetp.cond, sign, op_a, op_b);
        const Node second_pred = GetPredicate(instr.vsetp.pred39, false);

        const OperationCode combiner = GetPredicateCombiner(instr.vsetp.op);

        // Set the primary predicate to the result of Predicate OP SecondPredicate
        SetPredicate(bb, instr.vsetp.pred3, Operation(combiner, first_pred, second_pred));

        if (instr.vsetp.pred0 != static_cast<u64>(Pred::UnusedIndex)) {
            // Set the secondary predicate to the result of !Predicate OP SecondPredicate,
            // if enabled
            const Node negate_pred = Operation(OperationCode::LogicalNegate, first_pred);
            SetPredicate(bb, instr.vsetp.pred0, Operation(combiner, negate_pred, second_pred));
        }
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled video instruction: {}", opcode->get().GetName());
    }

    return pc;
}

Node ShaderIR::GetVideoOperand(Node op, bool is_chunk, bool is_signed, VideoType type,
                               u64 byte_height) {
    if (!is_chunk) {
        return BitfieldExtract(op, static_cast<u32>(byte_height * 8), 8);
    }

    switch (type) {
    case VideoType::Size16_Low:
        return BitfieldExtract(op, 0, 16);
    case VideoType::Size16_High:
        return BitfieldExtract(op, 16, 16);
    case VideoType::Size32:
        // TODO(Rodrigo): From my hardware tests it becomes a bit "mad" when this type is used
        // (1 * 1 + 0 == 0x5b800000). Until a better explanation is found: abort.
        UNIMPLEMENTED();
        return Immediate(0);
    case VideoType::Invalid:
        UNREACHABLE_MSG("Invalid instruction encoding");
        return Immediate(0);
    default:
        UNREACHABLE();
        return Immediate(0);
    }
}

void ShaderIR::DecodeVMNMX(NodeBlock& bb, Tegra::Shader::Instruction instr) {
    UNIMPLEMENTED_IF(!instr.vmnmx.is_op_b_register);
    UNIMPLEMENTED_IF(instr.vmnmx.SourceFormatA() != VmnmxType::Bits32);
    UNIMPLEMENTED_IF(instr.vmnmx.SourceFormatB() != VmnmxType::Bits32);
    UNIMPLEMENTED_IF(instr.vmnmx.is_src_a_signed != instr.vmnmx.is_src_b_signed);
    UNIMPLEMENTED_IF(instr.vmnmx.sat);
    UNIMPLEMENTED_IF(instr.generates_cc);

    Node op_a = GetRegister(instr.gpr8);
    Node op_b = GetRegister(instr.gpr20);
    Node op_c = GetRegister(instr.gpr39);

    const bool is_oper1_signed = instr.vmnmx.is_src_a_signed; // Stubbed
    const bool is_oper2_signed = instr.vmnmx.is_dest_signed;

    const auto operation_a = instr.vmnmx.mx ? OperationCode::IMax : OperationCode::IMin;
    Node value = SignedOperation(operation_a, is_oper1_signed, move(op_a), move(op_b));

    switch (instr.vmnmx.operation) {
    case VmnmxOperation::Mrg_16H:
        value = BitfieldInsert(move(op_c), move(value), 16, 16);
        break;
    case VmnmxOperation::Mrg_16L:
        value = BitfieldInsert(move(op_c), move(value), 0, 16);
        break;
    case VmnmxOperation::Mrg_8B0:
        value = BitfieldInsert(move(op_c), move(value), 0, 8);
        break;
    case VmnmxOperation::Mrg_8B2:
        value = BitfieldInsert(move(op_c), move(value), 16, 8);
        break;
    case VmnmxOperation::Acc:
        value = Operation(OperationCode::IAdd, move(value), move(op_c));
        break;
    case VmnmxOperation::Min:
        value = SignedOperation(OperationCode::IMin, is_oper2_signed, move(value), move(op_c));
        break;
    case VmnmxOperation::Max:
        value = SignedOperation(OperationCode::IMax, is_oper2_signed, move(value), move(op_c));
        break;
    case VmnmxOperation::Nop:
        break;
    default:
        UNREACHABLE();
        break;
    }

    SetRegister(bb, instr.gpr0, move(value));
}

} // namespace VideoCommon::Shader
