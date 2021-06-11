/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "common_types.h"
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::OpAtomicLoad(Id result_type, Id pointer, Id memory, Id semantics) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicLoad, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    return AddCode(std::move(op));
}

Id Module::OpAtomicStore(Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicStore)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicExchange(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicExchange, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicCompareExchange(Id result_type, Id pointer, Id memory, Id equal, Id unequal,
                                   Id value, Id comparator) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicCompareExchange, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(equal);
    op->Add(unequal);
    op->Add(value);
    op->Add(comparator);
    return AddCode(std::move(op));
}

Id Module::OpAtomicIIncrement(Id result_type, Id pointer, Id memory, Id semantics) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicIIncrement, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    return AddCode(std::move(op));
}

Id Module::OpAtomicIDecrement(Id result_type, Id pointer, Id memory, Id semantics) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicIDecrement, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    return AddCode(std::move(op));
}

Id Module::OpAtomicIAdd(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicIAdd, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicISub(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicISub, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicSMin(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicSMin, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicUMin(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicUMin, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicSMax(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicSMax, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicUMax(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicUMax, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicAnd(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicAnd, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicOr(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicOr, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpAtomicXor(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpAtomicXor, bound++, result_type)};
    op->Add(pointer);
    op->Add(memory);
    op->Add(semantics);
    op->Add(value);
    return AddCode(std::move(op));
}

} // namespace Sirit
