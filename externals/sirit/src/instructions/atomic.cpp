/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::OpAtomicLoad(Id result_type, Id pointer, Id memory, Id semantics) {
    code->Reserve(6);
    return *code << OpId{spv::Op::OpAtomicLoad, result_type} << pointer << memory << semantics
                 << EndOp{};
}

Id Module::OpAtomicStore(Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpAtomicStore} << pointer << memory << semantics << value
                 << EndOp{};
}

Id Module::OpAtomicExchange(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicExchange, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicCompareExchange(Id result_type, Id pointer, Id memory, Id equal, Id unequal,
                                   Id value, Id comparator) {
    code->Reserve(9);
    return *code << OpId{spv::Op::OpAtomicCompareExchange, result_type} << pointer << memory
                 << equal << unequal << value << comparator << EndOp{};
}

Id Module::OpAtomicIIncrement(Id result_type, Id pointer, Id memory, Id semantics) {
    code->Reserve(6);
    return *code << OpId{spv::Op::OpAtomicIIncrement, result_type} << pointer << memory << semantics
                 << EndOp{};
}

Id Module::OpAtomicIDecrement(Id result_type, Id pointer, Id memory, Id semantics) {
    code->Reserve(6);
    return *code << OpId{spv::Op::OpAtomicIDecrement, result_type} << pointer << memory << semantics
                 << EndOp{};
}

Id Module::OpAtomicIAdd(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicIAdd, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicISub(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicISub, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicSMin(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicSMin, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicUMin(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicUMin, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicSMax(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicSMax, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicUMax(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicUMax, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicAnd(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicAnd, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicOr(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicOr, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

Id Module::OpAtomicXor(Id result_type, Id pointer, Id memory, Id semantics, Id value) {
    code->Reserve(7);
    return *code << OpId{spv::Op::OpAtomicXor, result_type} << pointer << memory << semantics
                 << value << EndOp{};
}

} // namespace Sirit
