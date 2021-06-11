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

DEFINE_UNARY(OpAny, spv::Op::OpAny)
DEFINE_UNARY(OpAll, spv::Op::OpAll)
DEFINE_UNARY(OpIsNan, spv::Op::OpIsNan)
DEFINE_UNARY(OpIsInf, spv::Op::OpIsInf)
DEFINE_BINARY(OpLogicalEqual, spv::Op::OpLogicalEqual)
DEFINE_BINARY(OpLogicalNotEqual, spv::Op::OpLogicalNotEqual)
DEFINE_BINARY(OpLogicalOr, spv::Op::OpLogicalOr)
DEFINE_BINARY(OpLogicalAnd, spv::Op::OpLogicalAnd)
DEFINE_UNARY(OpLogicalNot, spv::Op::OpLogicalNot)
DEFINE_TRINARY(OpSelect, spv::Op::OpSelect)
DEFINE_BINARY(OpIEqual, spv::Op::OpIEqual)
DEFINE_BINARY(OpINotEqual, spv::Op::OpINotEqual)
DEFINE_BINARY(OpUGreaterThan, spv::Op::OpUGreaterThan)
DEFINE_BINARY(OpSGreaterThan, spv::Op::OpSGreaterThan)
DEFINE_BINARY(OpUGreaterThanEqual, spv::Op::OpUGreaterThanEqual)
DEFINE_BINARY(OpSGreaterThanEqual, spv::Op::OpSGreaterThanEqual)
DEFINE_BINARY(OpULessThan, spv::Op::OpULessThan)
DEFINE_BINARY(OpSLessThan, spv::Op::OpSLessThan)
DEFINE_BINARY(OpULessThanEqual, spv::Op::OpULessThanEqual)
DEFINE_BINARY(OpSLessThanEqual, spv::Op::OpSLessThanEqual)
DEFINE_BINARY(OpFOrdEqual, spv::Op::OpFOrdEqual)
DEFINE_BINARY(OpFUnordEqual, spv::Op::OpFUnordEqual)
DEFINE_BINARY(OpFOrdNotEqual, spv::Op::OpFOrdNotEqual)
DEFINE_BINARY(OpFUnordNotEqual, spv::Op::OpFUnordNotEqual)
DEFINE_BINARY(OpFOrdLessThan, spv::Op::OpFOrdLessThan)
DEFINE_BINARY(OpFUnordLessThan, spv::Op::OpFUnordLessThan)
DEFINE_BINARY(OpFOrdGreaterThan, spv::Op::OpFOrdGreaterThan)
DEFINE_BINARY(OpFUnordGreaterThan, spv::Op::OpFUnordGreaterThan)
DEFINE_BINARY(OpFOrdLessThanEqual, spv::Op::OpFOrdLessThanEqual)
DEFINE_BINARY(OpFUnordLessThanEqual, spv::Op::OpFUnordLessThanEqual)
DEFINE_BINARY(OpFOrdGreaterThanEqual, spv::Op::OpFOrdGreaterThanEqual)
DEFINE_BINARY(OpFUnordGreaterThanEqual, spv::Op::OpFUnordGreaterThanEqual)

} // namespace Sirit
