/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::OpShiftRightLogical(Id result_type, Id base, Id shift) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpShiftRightLogical, result_type} << base << shift << EndOp{};
}

Id Module::OpShiftRightArithmetic(Id result_type, Id base, Id shift) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpShiftRightArithmetic, result_type} << base << shift << EndOp{};
}

Id Module::OpShiftLeftLogical(Id result_type, Id base, Id shift) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpShiftLeftLogical, result_type} << base << shift << EndOp{};
}

Id Module::OpBitwiseOr(Id result_type, Id operand_1, Id operand_2) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpBitwiseOr, result_type} << operand_1 << operand_2 << EndOp{};
}

Id Module::OpBitwiseXor(Id result_type, Id operand_1, Id operand_2) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpBitwiseXor, result_type} << operand_1 << operand_2 << EndOp{};
}

Id Module::OpBitwiseAnd(Id result_type, Id operand_1, Id operand_2) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpBitwiseAnd, result_type} << operand_1 << operand_2 << EndOp{};
}

Id Module::OpNot(Id result_type, Id operand) {
    code->Reserve(4);
    return *code << OpId{spv::Op::OpNot, result_type} << operand << EndOp{};
}

Id Module::OpBitFieldInsert(Id result_type, Id base, Id insert, Id offset, Id count) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpBitFieldInsert, result_type} << base << insert << offset
                 << count << EndOp{};
}

Id Module::OpBitFieldSExtract(Id result_type, Id base, Id offset, Id count) {
    code->Reserve(6);
    return *code << OpId{spv::Op::OpBitFieldSExtract, result_type} << base << offset << count
                 << EndOp{};
}

Id Module::OpBitFieldUExtract(Id result_type, Id base, Id offset, Id count) {
    code->Reserve(6);
    return *code << OpId{spv::Op::OpBitFieldUExtract, result_type} << base << offset << count
                 << EndOp{};
}

Id Module::OpBitReverse(Id result_type, Id base) {
    code->Reserve(4);
    return *code << OpId{spv::Op::OpBitReverse, result_type} << base << EndOp{};
}

Id Module::OpBitCount(Id result_type, Id base) {
    code->Reserve(4);
    return *code << OpId{spv::Op::OpBitCount, result_type} << base << EndOp{};
}

} // namespace Sirit
