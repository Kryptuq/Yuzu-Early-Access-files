/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>
#include <vector>
#include "common_types.h"
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::OpLoopMerge(Id merge_block, Id continue_target, spv::LoopControlMask loop_control,
                       const std::vector<Id>& literals) {
    auto op{std::make_unique<Op>(spv::Op::OpLoopMerge)};
    op->Add(merge_block);
    op->Add(continue_target);
    op->Add(static_cast<u32>(loop_control));
    op->Add(literals);
    return AddCode(std::move(op));
}

Id Module::OpSelectionMerge(Id merge_block, spv::SelectionControlMask selection_control) {
    auto op{std::make_unique<Op>(spv::Op::OpSelectionMerge)};
    op->Add(merge_block);
    op->Add(static_cast<u32>(selection_control));
    return AddCode(std::move(op));
}

Id Module::OpLabel() {
    return code_store.emplace_back(std::make_unique<Op>(spv::Op::OpLabel, bound++)).get();
}

Id Module::OpBranch(Id target_label) {
    auto op{std::make_unique<Op>(spv::Op::OpBranch)};
    op->Add(target_label);
    return AddCode(std::move(op));
}

Id Module::OpBranchConditional(Id condition, Id true_label, Id false_label, u32 true_weight,
                               u32 false_weight) {
    auto op{std::make_unique<Op>(spv::Op::OpBranchConditional)};
    op->Add(condition);
    op->Add(true_label);
    op->Add(false_label);
    if (true_weight != 0 || false_weight != 0) {
        op->Add(true_weight);
        op->Add(false_weight);
    }
    return AddCode(std::move(op));
}

Id Module::OpSwitch(Id selector, Id default_label, const std::vector<Literal>& literals,
                    const std::vector<Id>& labels) {
    const std::size_t size = literals.size();
    assert(literals.size() == labels.size());
    auto op{std::make_unique<Op>(spv::Op::OpSwitch)};
    op->Add(selector);
    op->Add(default_label);
    for (std::size_t i = 0; i < size; ++i) {
        op->Add(literals[i]);
        op->Add(labels[i]);
    }
    return AddCode(std::move(op));
}

Id Module::OpReturn() {
    return AddCode(spv::Op::OpReturn);
}

Id Module::OpReturnValue(Id value) {
    auto op{std::make_unique<Op>(spv::Op::OpReturnValue)};
    op->Add(value);
    return AddCode(std::move(op));
}

Id Module::OpKill() {
    return AddCode(std::make_unique<Op>(spv::Op::OpKill));
}

} // namespace Sirit
