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

Id Module::OpShiftRightLogical(Id result_type, Id base, Id shift) {
    auto op{std::make_unique<Op>(spv::Op::OpShiftRightLogical, bound++, result_type)};
    op->Add(base);
    op->Add(shift);
    return AddCode(std::move(op));
}

Id Module::OpShiftRightArithmetic(Id result_type, Id base, Id shift) {
    auto op{std::make_unique<Op>(spv::Op::OpShiftRightArithmetic, bound++, result_type)};
    op->Add(base);
    op->Add(shift);
    return AddCode(std::move(op));
}

Id Module::OpShiftLeftLogical(Id result_type, Id base, Id shift) {
    auto op{std::make_unique<Op>(spv::Op::OpShiftLeftLogical, bound++, result_type)};
    op->Add(base);
    op->Add(shift);
    return AddCode(std::move(op));
}

Id Module::OpBitwiseOr(Id result_type, Id operand_1, Id operand_2) {
    auto op{std::make_unique<Op>(spv::Op::OpBitwiseOr, bound++, result_type)};
    op->Add(operand_1);
    op->Add(operand_2);
    return AddCode(std::move(op));
}

Id Module::OpBitwiseXor(Id result_type, Id operand_1, Id operand_2) {
    auto op{std::make_unique<Op>(spv::Op::OpBitwiseXor, bound++, result_type)};
    op->Add(operand_1);
    op->Add(operand_2);
    return AddCode(std::move(op));
}

Id Module::OpBitwiseAnd(Id result_type, Id operand_1, Id operand_2) {
    auto op{std::make_unique<Op>(spv::Op::OpBitwiseAnd, bound++, result_type)};
    op->Add(operand_1);
    op->Add(operand_2);
    return AddCode(std::move(op));
}

Id Module::OpNot(Id result_type, Id operand) {
    auto op{std::make_unique<Op>(spv::Op::OpNot, bound++, result_type)};
    op->Add(operand);
    return AddCode(std::move(op));
}

Id Module::OpBitFieldInsert(Id result_type, Id base, Id insert, Id offset, Id count) {
    auto op{std::make_unique<Op>(spv::Op::OpBitFieldInsert, bound++, result_type)};
    op->Add(base);
    op->Add(insert);
    op->Add(offset);
    op->Add(count);
    return AddCode(std::move(op));
}

Id Module::OpBitFieldSExtract(Id result_type, Id base, Id offset, Id count) {
    auto op{std::make_unique<Op>(spv::Op::OpBitFieldSExtract, bound++, result_type)};
    op->Add(base);
    op->Add(offset);
    op->Add(count);
    return AddCode(std::move(op));
}

Id Module::OpBitFieldUExtract(Id result_type, Id base, Id offset, Id count) {
    auto op{std::make_unique<Op>(spv::Op::OpBitFieldUExtract, bound++, result_type)};
    op->Add(base);
    op->Add(offset);
    op->Add(count);
    return AddCode(std::move(op));
}

Id Module::OpBitReverse(Id result_type, Id base) {
    auto op{std::make_unique<Op>(spv::Op::OpBitReverse, bound++, result_type)};
    op->Add(base);
    return AddCode(std::move(op));
}

Id Module::OpBitCount(Id result_type, Id base) {
    auto op{std::make_unique<Op>(spv::Op::OpBitCount, bound++, result_type)};
    op->Add(base);
    return AddCode(std::move(op));
}

} // namespace Sirit
