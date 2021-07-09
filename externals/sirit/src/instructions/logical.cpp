/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

#define DEFINE_UNARY(opcode)                                                                       \
    Id Module::opcode(Id result_type, Id operand) {                                                \
        code->Reserve(4);                                                                          \
        return *code << OpId{spv::Op::opcode, result_type} << operand << EndOp{};                  \
    }

#define DEFINE_BINARY(opcode)                                                                      \
    Id Module::opcode(Id result_type, Id operand_1, Id operand_2) {                                \
        code->Reserve(5);                                                                          \
        return *code << OpId{spv::Op::opcode, result_type} << operand_1 << operand_2 << EndOp{};   \
    }

#define DEFINE_TRINARY(opcode)                                                                     \
    Id Module::opcode(Id result_type, Id operand_1, Id operand_2, Id operand_3) {                  \
        code->Reserve(6);                                                                          \
        return *code << OpId{spv::Op::opcode, result_type} << operand_1 << operand_2 << operand_3  \
                     << EndOp{};                                                                   \
    }

DEFINE_UNARY(OpAny)
DEFINE_UNARY(OpAll)
DEFINE_UNARY(OpIsNan)
DEFINE_UNARY(OpIsInf)
DEFINE_BINARY(OpLogicalEqual)
DEFINE_BINARY(OpLogicalNotEqual)
DEFINE_BINARY(OpLogicalOr)
DEFINE_BINARY(OpLogicalAnd)
DEFINE_UNARY(OpLogicalNot)
DEFINE_TRINARY(OpSelect)
DEFINE_BINARY(OpIEqual)
DEFINE_BINARY(OpINotEqual)
DEFINE_BINARY(OpUGreaterThan)
DEFINE_BINARY(OpSGreaterThan)
DEFINE_BINARY(OpUGreaterThanEqual)
DEFINE_BINARY(OpSGreaterThanEqual)
DEFINE_BINARY(OpULessThan)
DEFINE_BINARY(OpSLessThan)
DEFINE_BINARY(OpULessThanEqual)
DEFINE_BINARY(OpSLessThanEqual)
DEFINE_BINARY(OpFOrdEqual)
DEFINE_BINARY(OpFUnordEqual)
DEFINE_BINARY(OpFOrdNotEqual)
DEFINE_BINARY(OpFUnordNotEqual)
DEFINE_BINARY(OpFOrdLessThan)
DEFINE_BINARY(OpFUnordLessThan)
DEFINE_BINARY(OpFOrdGreaterThan)
DEFINE_BINARY(OpFUnordGreaterThan)
DEFINE_BINARY(OpFOrdLessThanEqual)
DEFINE_BINARY(OpFUnordLessThanEqual)
DEFINE_BINARY(OpFOrdGreaterThanEqual)
DEFINE_BINARY(OpFUnordGreaterThanEqual)

} // namespace Sirit
