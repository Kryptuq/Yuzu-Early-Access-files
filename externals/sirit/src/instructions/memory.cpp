/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::OpImageTexelPointer(Id result_type, Id image, Id coordinate, Id sample) {
    code->Reserve(6);
    return *code << OpId{spv::Op::OpImageTexelPointer, result_type} << image << coordinate << sample
                 << EndOp{};
}

Id Module::OpLoad(Id result_type, Id pointer, std::optional<spv::MemoryAccessMask> memory_access) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpLoad, result_type} << pointer << memory_access << EndOp{};
}

Id Module::OpStore(Id pointer, Id object, std::optional<spv::MemoryAccessMask> memory_access) {
    code->Reserve(4);
    return *code << spv::Op::OpStore << pointer << object << memory_access << EndOp{};
}

Id Module::OpAccessChain(Id result_type, Id base, std::span<const Id> indexes) {
    assert(!indexes.empty());
    code->Reserve(4 + indexes.size());
    return *code << OpId{spv::Op::OpAccessChain, result_type} << base << indexes << EndOp{};
}

Id Module::OpVectorExtractDynamic(Id result_type, Id vector, Id index) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpVectorExtractDynamic, result_type} << vector << index
                 << EndOp{};
}

Id Module::OpVectorInsertDynamic(Id result_type, Id vector, Id component, Id index) {
    code->Reserve(6);
    return *code << OpId{spv::Op::OpVectorInsertDynamic, result_type} << vector << component
                 << index << EndOp{};
}

Id Module::OpCompositeInsert(Id result_type, Id object, Id composite,
                             std::span<const Literal> indexes) {
    code->Reserve(5 + indexes.size());
    return *code << OpId{spv::Op::OpCompositeInsert, result_type} << object << composite << indexes
                 << EndOp{};
}

Id Module::OpCompositeExtract(Id result_type, Id composite, std::span<const Literal> indexes) {
    code->Reserve(4 + indexes.size());
    return *code << OpId{spv::Op::OpCompositeExtract, result_type} << composite << indexes
                 << EndOp{};
}

Id Module::OpCompositeConstruct(Id result_type, std::span<const Id> ids) {
    assert(ids.size() >= 1);
    code->Reserve(3 + ids.size());
    return *code << OpId{spv::Op::OpCompositeConstruct, result_type} << ids << EndOp{};
}

} // namespace Sirit
