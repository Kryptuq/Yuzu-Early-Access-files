/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <memory>
#include "common_types.h"
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

#define DEFINE_UNARY(funcname, opcode)                                                             \
    Id Module::funcname(Id result_type, Id operand) {                                              \
        auto op{std::make_unique<Op>(opcode, bound++, result_type)};                               \
        op->Add(operand);                                                                          \
        return AddCode(std::move(op));                                                             \
    }

#define DEFINE_BINARY(funcname, opcode)                                                            \
    Id Module::funcname(Id result_type, Id operand_1, Id operand_2) {                              \
        auto op{std::make_unique<Op>(opcode, bound++, result_type)};                               \
        op->Add(operand_1);                                                                        \
        op->Add(operand_2);                                                                        \
        return AddCode(std::move(op));                                                             \
    }

#define DEFINE_TRINARY(funcname, opcode)                                                           \
    Id Module::funcname(Id result_type, Id operand_1, Id operand_2, Id operand_3) {                \
        auto op{std::make_unique<Op>(opcode, bound++, result_type)};                               \
        op->Add(operand_1);                                                                        \
        op->Add(operand_2);                                                                        \
        op->Add(operand_3);                                                                        \
        return AddCode(std::move(op));                                                             \
    }

DEFINE_UNARY(OpSNegate, spv::Op::OpSNegate)
DEFINE_UNARY(OpFNegate, spv::Op::OpFNegate)

DEFINE_BINARY(OpIAdd, spv::Op::OpIAdd)
DEFINE_BINARY(OpFAdd, spv::Op::OpFAdd)
DEFINE_BINARY(OpISub, spv::Op::OpISub)
DEFINE_BINARY(OpFSub, spv::Op::OpFSub)
DEFINE_BINARY(OpIMul, spv::Op::OpIMul)
DEFINE_BINARY(OpFMul, spv::Op::OpFMul)
DEFINE_BINARY(OpUDiv, spv::Op::OpUDiv)
DEFINE_BINARY(OpSDiv, spv::Op::OpSDiv)
DEFINE_BINARY(OpFDiv, spv::Op::OpFDiv)
DEFINE_BINARY(OpUMod, spv::Op::OpUMod)
DEFINE_BINARY(OpSMod, spv::Op::OpSMod)
DEFINE_BINARY(OpFMod, spv::Op::OpFMod)
DEFINE_BINARY(OpSRem, spv::Op::OpSRem)
DEFINE_BINARY(OpFRem, spv::Op::OpFRem)
DEFINE_BINARY(OpIAddCarry, spv::Op::OpIAddCarry)

} // namespace Sirit
