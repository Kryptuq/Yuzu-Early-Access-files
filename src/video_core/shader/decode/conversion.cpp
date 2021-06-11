// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <limits>
#include <optional>
#include <utility>

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::Register;

namespace {

constexpr OperationCode GetFloatSelector(u64 selector) {
    return selector == 0 ? OperationCode::FCastHalf0 : OperationCode::FCastHalf1;
}

constexpr u32 SizeInBits(Register::Size size) {
    switch (size) {
    case Register::Size::Byte:
        return 8;
    case Register::Size::Short:
        return 16;
    case Register::Size::Word:
        return 32;
    case Register::Size::Long:
        return 64;
    }
    return 0;
}

constexpr std::optional<std::pair<s32, s32>> IntegerSaturateBounds(Register::Size src_size,
                                                                   Register::Size dst_size,
                                                                   bool src_signed,
                                                                   bool dst_signed) {
    const u32 dst_bits = SizeInBits(dst_size);
    if (src_size == Register::Size::Word && dst_size == Register::Size::Word) {
        if (src_signed == dst_signed) {
            return std::nullopt;
        }
        return std::make_pair(0, std::numeric_limits<s32>::max());
    }
    if (dst_signed) {
        // Signed destination, clamp to [-128, 127] for instance
        return std::make_pair(-(1 << (dst_bits - 1)), (1 << (dst_bits - 1)) - 1);
    } else {
        // Unsigned destination
        if (dst_bits == 32) {
            // Avoid shifting by 32, that is undefined behavior
            return std::make_pair(0, s32(std::numeric_limits<u32>::max()));
        }
        return std::make_pair(0, (1 << dst_bits) - 1);
    }
}

} // Anonymous namespace

u32 ShaderIR::DecodeConversion(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    switch (opcode->get().GetId()) {
    case OpCode::Id::I2I_R:
    case OpCode::Id::I2I_C:
    case OpCode::Id::I2I_IMM: {
        const bool src_signed = instr.conversion.is_input_signed;
        const bool dst_signed = instr.conversion.is_output_signed;
        const Register::Size src_size = instr.conversion.src_size;
        const Register::Size dst_size = instr.conversion.dst_size;
        const u32 selector = static_cast<u32>(instr.conversion.int_src.selector);

        Node value = [this, instr, opcode] {
            switch (opcode->get().GetId()) {
            case OpCode::Id::I2I_R:
                return GetRegister(instr.gpr20);
            case OpCode::Id::I2I_C:
                return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
            case OpCode::Id::I2I_IMM:
                return Immediate(instr.alu.GetSignedImm20_20());
            default:
                UNREACHABLE();
                return Immediate(0);
            }
        }();

        // Ensure the source selector is valid
        switch (instr.conversion.src_size) {
        case Register::Size::Byte:
            break;
        case Register::Size::Short:
            ASSERT(selector == 0 || selector == 2);
            break;
        default:
            ASSERT(selector == 0);
            break;
        }

        if (src_size != Register::Size::Word || selector != 0) {
            value = SignedOperation(OperationCode::IBitfieldExtract, src_signed, std::move(value),
                                    Immediate(selector * 8), Immediate(SizeInBits(src_size)));
        }

        value = GetOperandAbsNegInteger(std::move(value), instr.conversion.abs_a,
                                        instr.conversion.negate_a, src_signed);

        if (instr.alu.saturate_d) {
            if (src_signed && !dst_signed) {
                Node is_negative = Operation(OperationCode::LogicalUGreaterEqual, value,
                                             Immediate(1 << (SizeInBits(src_size) - 1)));
                value = Operation(OperationCode::Select, std::move(is_negative), Immediate(0),
                                  std::move(value));

                // Simplify generated expressions, this can be removed without semantic impact
                SetTemporary(bb, 0, std::move(value));
                value = GetTemporary(0);

                if (dst_size != Register::Size::Word) {
                    const Node limit = Immediate((1 << SizeInBits(dst_size)) - 1);
                    Node is_large =
                        Operation(OperationCode::LogicalUGreaterThan, std::move(value), limit);
                    value = Operation(OperationCode::Select, std::move(is_large), limit,
                                      std::move(value));
                }
            } else if (const std::optional bounds =
                           IntegerSaturateBounds(src_size, dst_size, src_signed, dst_signed)) {
                value = SignedOperation(OperationCode::IMax, src_signed, std::move(value),
                                        Immediate(bounds->first));
                value = SignedOperation(OperationCode::IMin, src_signed, std::move(value),
                                        Immediate(bounds->second));
            }
        } else if (dst_size != Register::Size::Word) {
            // No saturation, we only have to mask the result
            Node mask = Immediate((1 << SizeInBits(dst_size)) - 1);
            value = Operation(OperationCode::UBitwiseAnd, std::move(value), std::move(mask));
        }

        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
        SetRegister(bb, instr.gpr0, std::move(value));
        break;
    }
    case OpCode::Id::I2F_R:
    case OpCode::Id::I2F_C:
    case OpCode::Id::I2F_IMM: {
        UNIMPLEMENTED_IF(instr.conversion.dst_size == Register::Size::Long);
        UNIMPLEMENTED_IF_MSG(instr.generates_cc,
                             "Condition codes generation in I2F is not implemented");

        Node value = [&] {
            switch (opcode->get().GetId()) {
            case OpCode::Id::I2F_R:
                return GetRegister(instr.gpr20);
            case OpCode::Id::I2F_C:
                return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
            case OpCode::Id::I2F_IMM:
                return Immediate(instr.alu.GetSignedImm20_20());
            default:
                UNREACHABLE();
                return Immediate(0);
            }
        }();

        const bool input_signed = instr.conversion.is_input_signed;

        if (const u32 offset = static_cast<u32>(instr.conversion.int_src.selector); offset > 0) {
            ASSERT(instr.conversion.src_size == Register::Size::Byte ||
                   instr.conversion.src_size == Register::Size::Short);
            if (instr.conversion.src_size == Register::Size::Short) {
                ASSERT(offset == 0 || offset == 2);
            }
            value = SignedOperation(OperationCode::ILogicalShiftRight, input_signed,
                                    std::move(value), Immediate(offset * 8));
        }

        value = ConvertIntegerSize(value, instr.conversion.src_size, input_signed);
        value = GetOperandAbsNegInteger(value, instr.conversion.abs_a, false, input_signed);
        value = SignedOperation(OperationCode::FCastInteger, input_signed, PRECISE, value);
        value = GetOperandAbsNegFloat(value, false, instr.conversion.negate_a);

        SetInternalFlagsFromFloat(bb, value, instr.generates_cc);

        if (instr.conversion.dst_size == Register::Size::Short) {
            value = Operation(OperationCode::HCastFloat, PRECISE, value);
        }

        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::F2F_R:
    case OpCode::Id::F2F_C:
    case OpCode::Id::F2F_IMM: {
        UNIMPLEMENTED_IF(instr.conversion.dst_size == Register::Size::Long);
        UNIMPLEMENTED_IF(instr.conversion.src_size == Register::Size::Long);
        UNIMPLEMENTED_IF_MSG(instr.generates_cc,
                             "Condition codes generation in F2F is not implemented");

        Node value = [&]() {
            switch (opcode->get().GetId()) {
            case OpCode::Id::F2F_R:
                return GetRegister(instr.gpr20);
            case OpCode::Id::F2F_C:
                return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
            case OpCode::Id::F2F_IMM:
                return GetImmediate19(instr);
            default:
                UNREACHABLE();
                return Immediate(0);
            }
        }();

        if (instr.conversion.src_size == Register::Size::Short) {
            value = Operation(GetFloatSelector(instr.conversion.float_src.selector), NO_PRECISE,
                              std::move(value));
        } else {
            ASSERT(instr.conversion.float_src.selector == 0);
        }

        value = GetOperandAbsNegFloat(value, instr.conversion.abs_a, instr.conversion.negate_a);

        value = [&] {
            if (instr.conversion.src_size != instr.conversion.dst_size) {
                // Rounding operations only matter when the source and destination conversion size
                // is the same.
                return value;
            }
            switch (instr.conversion.f2f.GetRoundingMode()) {
            case Tegra::Shader::F2fRoundingOp::None:
                return value;
            case Tegra::Shader::F2fRoundingOp::Round:
                return Operation(OperationCode::FRoundEven, value);
            case Tegra::Shader::F2fRoundingOp::Floor:
                return Operation(OperationCode::FFloor, value);
            case Tegra::Shader::F2fRoundingOp::Ceil:
                return Operation(OperationCode::FCeil, value);
            case Tegra::Shader::F2fRoundingOp::Trunc:
                return Operation(OperationCode::FTrunc, value);
            default:
                UNIMPLEMENTED_MSG("Unimplemented F2F rounding mode {}",
                                  instr.conversion.f2f.rounding.Value());
                return value;
            }
        }();
        value = GetSaturatedFloat(value, instr.alu.saturate_d);

        SetInternalFlagsFromFloat(bb, value, instr.generates_cc);

        if (instr.conversion.dst_size == Register::Size::Short) {
            value = Operation(OperationCode::HCastFloat, PRECISE, value);
        }

        SetRegister(bb, instr.gpr0, value);
        break;
    }
    case OpCode::Id::F2I_R:
    case OpCode::Id::F2I_C:
    case OpCode::Id::F2I_IMM: {
        UNIMPLEMENTED_IF(instr.conversion.src_size == Register::Size::Long);
        UNIMPLEMENTED_IF_MSG(instr.generates_cc,
                             "Condition codes generation in F2I is not implemented");
        Node value = [&]() {
            switch (opcode->get().GetId()) {
            case OpCode::Id::F2I_R:
                return GetRegister(instr.gpr20);
            case OpCode::Id::F2I_C:
                return GetConstBuffer(instr.cbuf34.index, instr.cbuf34.GetOffset());
            case OpCode::Id::F2I_IMM:
                return GetImmediate19(instr);
            default:
                UNREACHABLE();
                return Immediate(0);
            }
        }();

        if (instr.conversion.src_size == Register::Size::Short) {
            value = Operation(GetFloatSelector(instr.conversion.float_src.selector), NO_PRECISE,
                              std::move(value));
        } else {
            ASSERT(instr.conversion.float_src.selector == 0);
        }

        value = GetOperandAbsNegFloat(value, instr.conversion.abs_a, instr.conversion.negate_a);

        value = [&]() {
            switch (instr.conversion.f2i.rounding) {
            case Tegra::Shader::F2iRoundingOp::RoundEven:
                return Operation(OperationCode::FRoundEven, PRECISE, value);
            case Tegra::Shader::F2iRoundingOp::Floor:
                return Operation(OperationCode::FFloor, PRECISE, value);
            case Tegra::Shader::F2iRoundingOp::Ceil:
                return Operation(OperationCode::FCeil, PRECISE, value);
            case Tegra::Shader::F2iRoundingOp::Trunc:
                return Operation(OperationCode::FTrunc, PRECISE, value);
            default:
                UNIMPLEMENTED_MSG("Unimplemented F2I rounding mode {}",
                                  instr.conversion.f2i.rounding.Value());
                return Immediate(0);
            }
        }();
        const bool is_signed = instr.conversion.is_output_signed;
        value = SignedOperation(OperationCode::ICastFloat, is_signed, PRECISE, value);
        value = ConvertIntegerSize(value, instr.conversion.dst_size, is_signed);

        SetRegister(bb, instr.gpr0, value);
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled conversion instruction: {}", opcode->get().GetName());
    }

    return pc;
}

} // namespace VideoCommon::Shader
