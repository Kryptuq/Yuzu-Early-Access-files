/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::OpVariable(Id result_type, spv::StorageClass storage_class, Id initializer) {
    auto op{std::make_unique<Op>(spv::Op::OpVariable, bound++, result_type)};
    op->Add(static_cast<u32>(storage_class));
    if (initializer) {
        op->Add(initializer);
    }
    return code_store.emplace_back(std::move(op)).get();
}

Id Module::OpImageTexelPointer(Id result_type, Id image, Id coordinate, Id sample) {
    auto op{std::make_unique<Op>(spv::Op::OpImageTexelPointer, bound++, result_type)};
    op->Add(image);
    op->Add(coordinate);
    op->Add(sample);
    return AddCode(std::move(op));
}

Id Module::OpLoad(Id result_type, Id pointer, std::optional<spv::MemoryAccessMask> memory_access) {
    auto op{std::make_unique<Op>(spv::Op::OpLoad, bound++, result_type)};
    op->Add(pointer);
    if (memory_access) {
        op->Add(static_cast<u32>(*memory_access));
    }
    return AddCode(std::move(op));
}

Id Module::OpStore(Id pointer, Id object, std::optional<spv::MemoryAccessMask> memory_access) {
    auto op{std::make_unique<Op>(spv::Op::OpStore)};
    op->Add(pointer);
    op->Add(object);
    if (memory_access) {
        op->Add(static_cast<u32>(*memory_access));
    }
    return AddCode(std::move(op));
}

Id Module::OpAccessChain(Id result_type, Id base, const std::vector<Id>& indexes) {
    assert(indexes.size() > 0);
    auto op{std::make_unique<Op>(spv::Op::OpAccessChain, bound++, result_type)};
    op->Add(base);
    op->Add(indexes);
    return AddCode(std::move(op));
}

Id Module::OpVectorExtractDynamic(Id result_type, Id vector, Id index) {
    auto op{std::make_unique<Op>(spv::Op::OpVectorExtractDynamic, bound++, result_type)};
    op->Add(vector);
    op->Add(index);
    return AddCode(std::move(op));
}

Id Module::OpVectorInsertDynamic(Id result_type, Id vector, Id component, Id index) {
    auto op{std::make_unique<Op>(spv::Op::OpVectorInsertDynamic, bound++, result_type)};
    op->Add(vector);
    op->Add(component);
    op->Add(index);
    return AddCode(std::move(op));
}

Id Module::OpCompositeInsert(Id result_type, Id object, Id composite,
                             const std::vector<Literal>& indexes) {
    auto op{std::make_unique<Op>(spv::Op::OpCompositeInsert, bound++, result_type)};
    op->Add(object);
    op->Add(composite);
    op->Add(indexes);
    return AddCode(std::move(op));
}

Id Module::OpCompositeExtract(Id result_type, Id composite, const std::vector<Literal>& indexes) {
    auto op{std::make_unique<Op>(spv::Op::OpCompositeExtract, bound++, result_type)};
    op->Add(composite);
    op->Add(indexes);
    return AddCode(std::move(op));
}

Id Module::OpCompositeConstruct(Id result_type, const std::vector<Id>& ids) {
    assert(ids.size() >= 1);
    auto op{std::make_unique<Op>(spv::Op::OpCompositeConstruct, bound++, result_type)};
    op->Add(ids);
    return AddCode(std::move(op));
}

} // namespace Sirit
