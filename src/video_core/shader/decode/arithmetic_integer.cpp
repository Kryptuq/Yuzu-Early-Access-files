// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::IAdd3Height;
using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::Pred;
using Tegra::Shader::Register;

u32 ShaderIR::DecodeArithmeticInteger(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    Node op_a = GetRegister(instr.gpr8);
    Node op_b = [&]() {
        if (instr.is_b_imm) {
            return Immediate(instr.alu.GetSignedImm20_20());
        } else if (instr.is_b_gpr) {
            return GetRegister(instr.gpr20);
        } else {
            return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
        }
    }();

    switch (opcode->get().GetId()) {
    case OpCode::Id::IADD_C:
    case OpCode::Id::IADD_R:
    case OpCode::Id::IADD_IMM: {
        UNIMPLEMENTED_IF_MSG(instr.alu.saturate_d, "IADD.SAT");
        UNIMPLEMENTED_IF_MSG(instr.iadd.x && instr.generates_cc, "IADD.X Rd.CC");

        op_a = GetOperandAbsNegInteger(op_a, false, instr.alu_integer.negate_a, true);
        op_b = GetOperandAbsNegInteger(op_b, false, instr.alu_integer.negate_b, true);

        Node value = Operation(OperationCode::UAdd, op_a, op_b);

        if (instr.iadd.x) {
            Node carry = GetInternalFlag(InternalFlag::Carry);
            Node x = Operation(OperationCode::Select, std::move(carry), Immediate(1), Immediate(0));
            value = Operation(OperationCode::UAdd, std::move(value), std::move(x));
        }

        if (instr.generates_cc) {
            const Node i0 = Immediate(0);

            Node zero = Operation(OperationCode::LogicalIEqual, value, i0);
            Node sign = Operation(OperationCode::LogicalILessThan, value, i0);
            Node carry = Operation(OperationCode::LogicalAddCarry, op_a, op_b);

            Node pos_a = Operation(OperationCode::LogicalIGreaterThan, op_a, i0);
            Node pos_b = Operation(OperationCode::LogicalIGreaterThan, op_b, i0);
            Node pos = Operation(OperationCode::LogicalAnd, std::move(pos_a), std::move(pos_b));
            Node overflow = Operation(OperationCode::LogicalAnd, pos, sign);

            SetInternalFlag(bb, InternalFlag::Zero, std::move(zero));
            SetInternalFlag(bb, InternalFlag::Sign, std::move(sign));
            SetInternalFlag(bb, InternalFlag::Carry, std::move(carry));
            SetInternalFlag(bb, InternalFlag::Overflow, std::move(overflow));
        }
        SetRegister(bb, instr.gpr0, std::move(value));
        break;
    }
    case OpCode::Id::IADD3_C:
    case OpCode::Id::IADD3_R:
    case OpCode::Id::IADD3_IMM: {
        Node op_c = GetRegister(instr.gpr39);

        const auto ApplyHeight = [&](IAdd3Height height, Node value) {
            switch (height) {
            case IAdd3Height::None:
                return value;
            case IAdd3Height::LowerHalfWord:
                return BitfieldExtract(value, 0, 16);
            case IAdd3Height::UpperHalfWord:
                return BitfieldExtract(value, 16, 16);
            default:
                UNIMPLEMENTED_MSG("Unhandled IADD3 height: {}", height);
                return Immediate(0);
            }
        };

        if (opcode->get().GetId() == OpCode::Id::IADD3_R) {
            op_a = ApplyHeight(instr.iadd3.height_a, op_a);
            op_b = ApplyHeight(instr.iadd3.height_b, op_b);
            op_c = ApplyHeight(instr.iadd3.height_c, op_c);
        }

        op_a = GetOperandAbsNegInteger(op_a, false, instr.iadd3.neg_a, true);
        op_b = GetOperandAbsNegInteger(op_b, false, instr.iadd3.neg_b, true);
        op_c = GetOperandAbsNegInteger(op_c, false, instr.iadd3.neg_c, true);

        const Node value = [&] {
            Node add_ab = Operation(OperationCode::IAdd, NO_PRECISE, op_a, op_b);
            if (opcode->get().GetId() != OpCode::Id::IADD3_R) {
                return Operation(OperationCode::IAdd, NO_PRECISE, add_ab, op_c);
            }
            const Node shifted = [&] {
                switch (instr.iadd3.mode) {
                case Tegra::Shader::IAdd3Mode::RightShift:
                    // TODO(tech4me): According to
                    // https://envytools.readthedocs.io/en/latest/hw/graph/maxwell/cuda/int.html?highlight=iadd3
                    // The addition between op_a and op_b should be done in uint33, more
                    // investigation required
                    return Operation(OperationCode::ILogicalShiftRight, NO_PRECISE, add_ab,
                                     Immediate(16));
                case Tegra::Shader::IAdd3Mode::LeftShift:
                    return Operation(OperationCode::ILogicalShiftLeft, NO_PRECISE, add_ab,
                                     Immediate(16));
                default:
                    return add_ab;
                }
            }();
            return Operation(OperationCode::IAdd, NO_PRECISE, shifted, op_c);
        }();

        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::ISCADD_C:
    case OpCode::Id::ISCADD_R:
    case OpCode::Id::ISCADD_IMM: {
        UNIMPLEMENTED_IF_MSG(instr.generates_cc,
                             "Condition codes generation in ISCADD is not implemented");

        op_a = GetOperandAbsNegInteger(op_a, false, instr.alu_integer.negate_a, true);
        op_b = GetOperandAbsNegInteger(op_b, false, instr.alu_integer.negate_b, true);

        const Node shift = Immediate(static_cast<u32>(instr.alu_integer.shift_amount));
        const Node shifted_a = Operation(OperationCode::ILogicalShiftLeft, NO_PRECISE, op_a, shift);
        const Node value = Operation(OperationCode::IAdd, NO_PRECISE, shifted_a, op_b);

        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::POPC_C:
    case OpCode::Id::POPC_R:
    case OpCode::Id::POPC_IMM: {
        if (instr.popc.invert) {
            op_b = Operation(OperationCode::IBitwiseNot, NO_PRECISE, op_b);
        }
        const Node value = Operation(OperationCode::IBitCount, PRECISE, op_b);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::FLO_R:
    case OpCode::Id::FLO_C:
    case OpCode::Id::FLO_IMM: {
        Node value;
        if (instr.flo.invert) {
            op_b = Operation(OperationCode::IBitwiseNot, NO_PRECISE, std::move(op_b));
        }
        if (instr.flo.is_signed) {
            value = Operation(OperationCode::IBitMSB, NO_PRECISE, std::move(op_b));
        } else {
            value = Operation(OperationCode::UBitMSB, NO_PRECISE, std::move(op_b));
        }
        if (instr.flo.sh) {
            value =
                Operation(OperationCode::UBitwiseXor, NO_PRECISE, std::move(value), Immediate(31));
        }
        SetRegister(bb, instr.gpr0, std::move(value));
        break;
    }
    case OpCode::Id::SEL_C:
    case OpCode::Id::SEL_R:
    case OpCode::Id::SEL_IMM: {
        const Node condition = GetPredicate(instr.sel.pred, instr.sel.neg_pred != 0);
        const Node value = Operation(OperationCode::Select, PRECISE, condition, op_a, op_b);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::ICMP_CR:
    case OpCode::Id::ICMP_R:
    case OpCode::Id::ICMP_RC:
    case OpCode::Id::ICMP_IMM: {
        const Node zero = Immediate(0);

        const auto [op_rhs, test] = [&]() -> std::pair<Node, Node> {
            switch (opcode->get().GetId()) {
            case OpCode::Id::ICMP_CR:
                return {GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset()),
                        GetRegister(instr.gpr39)};
            case OpCode::Id::ICMP_R:
                return {GetRegister(instr.gpr20), GetRegister(instr.gpr39)};
            case OpCode::Id::ICMP_RC:
                return {GetRegister(instr.gpr39),
                        GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset())};
            case OpCode::Id::ICMP_IMM:
                return {Immediate(instr.alu.GetSignedImm20_20()), GetRegister(instr.gpr39)};
            default:
                UNREACHABLE();
                return {zero, zero};
            }
        }();
        const Node op_lhs = GetRegister(instr.gpr8);
        const Node comparison =
            GetPredicateComparisonInteger(instr.icmp.cond, instr.icmp.is_signed != 0, test, zero);
        SetRegister(bb, instr.gpr0, Operation(OperationCode::Select, comparison, op_lhs, op_rhs));
        break;
    }
    case OpCode::Id::LOP_C:
    case OpCode::Id::LOP_R:
    case OpCode::Id::LOP_IMM: {
        if (instr.alu.lop.invert_a)
            op_a = Operation(OperationCode::IBitwiseNot, NO_PRECISE, op_a);
        if (instr.alu.lop.invert_b)
            op_b = Operation(OperationCode::IBitwiseNot, NO_PRECISE, op_b);

        WriteLogicOperation(bb, instr.gpr0, instr.alu.lop.operation, op_a, op_b,
                            instr.alu.lop.pred_result_mode, instr.alu.lop.pred48,
                            instr.generates_cc);
        break;
    }
    case OpCode::Id::LOP3_C:
    case OpCode::Id::LOP3_R:
    case OpCode::Id::LOP3_IMM: {
        const Node op_c = GetRegister(instr.gpr39);
        const Node lut = [&]() {
            if (opcode->get().GetId() == OpCode::Id::LOP3_R) {
                return Immediate(instr.alu.lop3.GetImmLut28());
            } else {
                return Immediate(instr.alu.lop3.GetImmLut48());
            }
        }();

        WriteLop3Instruction(bb, instr.gpr0, op_a, op_b, op_c, lut, instr.generates_cc);
        break;
    }
    case OpCode::Id::IMNMX_C:
    case OpCode::Id::IMNMX_R:
    case OpCode::Id::IMNMX_IMM: {
        UNIMPLEMENTED_IF(instr.imnmx.exchange != Tegra::Shader::IMinMaxExchange::None);

        const bool is_signed = instr.imnmx.is_signed;

        const Node condition = GetPredicate(instr.imnmx.pred, instr.imnmx.negate_pred != 0);
        const Node min = SignedOperation(OperationCode::IMin, is_signed, NO_PRECISE, op_a, op_b);
        const Node max = SignedOperation(OperationCode::IMax, is_signed, NO_PRECISE, op_a, op_b);
        const Node value = Operation(OperationCode::Select, NO_PRECISE, condition, min, max);

        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::LEA_R2:
    case OpCode::Id::LEA_R1:
    case OpCode::Id::LEA_IMM:
    case OpCode::Id::LEA_RZ:
    case OpCode::Id::LEA_HI: {
        auto [op_a_, op_b_, op_c_] = [&]() -> std::tuple<Node, Node, Node> {
            switch (opcode->get().GetId()) {
            case OpCode::Id::LEA_R2: {
                return {GetRegister(instr.gpr20), GetRegister(instr.gpr39),
                        Immediate(static_cast<u32>(instr.lea.r2.entry_a))};
            }
            case OpCode::Id::LEA_R1: {
                const bool neg = instr.lea.r1.neg != 0;
                return {GetOperandAbsNegInteger(GetRegister(instr.gpr8), false, neg, true),
                        GetRegister(instr.gpr20),
                        Immediate(static_cast<u32>(instr.lea.r1.entry_a))};
            }
            case OpCode::Id::LEA_IMM: {
                const bool neg = instr.lea.imm.neg != 0;
                return {GetOperandAbsNegInteger(GetRegister(instr.gpr8), false, neg, true),
                        Immediate(static_cast<u32>(instr.lea.imm.entry_a)),
                        Immediate(static_cast<u32>(instr.lea.imm.entry_b))};
            }
            case OpCode::Id::LEA_RZ: {
                const bool neg = instr.lea.rz.neg != 0;
                return {GetConstBuffer(instr.lea.rz.cb_index, instr.lea.rz.cb_offset),
                        GetOperandAbsNegInteger(GetRegister(instr.gpr8), false, neg, true),
                        Immediate(static_cast<u32>(instr.lea.rz.entry_a))};
            }
            case OpCode::Id::LEA_HI:
            default:
                UNIMPLEMENTED_MSG("Unhandled LEA subinstruction: {}", opcode->get().GetName());

                return {Immediate(static_cast<u32>(instr.lea.imm.entry_a)), GetRegister(instr.gpr8),
                        Immediate(static_cast<u32>(instr.lea.imm.entry_b))};
            }
        }();

        UNIMPLEMENTED_IF_MSG(instr.lea.pred48 != static_cast<u64>(Pred::UnusedIndex),
                             "Unhandled LEA Predicate");

        Node value =
            Operation(OperationCode::ILogicalShiftLeft, std::move(op_a_), std::move(op_c_));
        value = Operation(OperationCode::IAdd, std::move(op_b_), std::move(value));
        SetRegister(bb, instr.gpr0, std::move(value));

        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled ArithmeticInteger instruction: {}", opcode->get().GetName());
    }

    return pc;
}

void ShaderIR::WriteLop3Instruction(NodeBlock& bb, Register dest, Node op_a, Node op_b, Node op_c,
                                    Node imm_lut, bool sets_cc) {
    const Node lop3_fast = [&](const Node na, const Node nb, const Node nc, const Node ttbl) {
        Node value = Immediate(0);
        const ImmediateNode imm = std::get<ImmediateNode>(*ttbl);
        if (imm.GetValue() & 0x01) {
            const Node a = Operation(OperationCode::IBitwiseNot, na);
            const Node b = Operation(OperationCode::IBitwiseNot, nb);
            const Node c = Operation(OperationCode::IBitwiseNot, nc);
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, a, b);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, c);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        if (imm.GetValue() & 0x02) {
            const Node a = Operation(OperationCode::IBitwiseNot, na);
            const Node b = Operation(OperationCode::IBitwiseNot, nb);
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, a, b);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, nc);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        if (imm.GetValue() & 0x04) {
            const Node a = Operation(OperationCode::IBitwiseNot, na);
            const Node c = Operation(OperationCode::IBitwiseNot, nc);
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, a, nb);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, c);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        if (imm.GetValue() & 0x08) {
            const Node a = Operation(OperationCode::IBitwiseNot, na);
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, a, nb);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, nc);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        if (imm.GetValue() & 0x10) {
            const Node b = Operation(OperationCode::IBitwiseNot, nb);
            const Node c = Operation(OperationCode::IBitwiseNot, nc);
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, na, b);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, c);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        if (imm.GetValue() & 0x20) {
            const Node b = Operation(OperationCode::IBitwiseNot, nb);
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, na, b);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, nc);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        if (imm.GetValue() & 0x40) {
            const Node c = Operation(OperationCode::IBitwiseNot, nc);
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, na, nb);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, c);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        if (imm.GetValue() & 0x80) {
            Node r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, na, nb);
            r = Operation(OperationCode::IBitwiseAnd, NO_PRECISE, r, nc);
            value = Operation(OperationCode::IBitwiseOr, value, r);
        }
        return value;
    }(op_a, op_b, op_c, imm_lut);

    SetInternalFlagsFromInteger(bb, lop3_fast, sets_cc);
    SetRegister(bb, dest, lop3_fast);
}

} // namespace VideoCommon::Shader
