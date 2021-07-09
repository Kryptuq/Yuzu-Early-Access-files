/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::OpPhi(Id result_type, std::span<const Id> operands) {
    assert(operands.size() % 2 == 0);
    code->Reserve(3 + operands.size());
    return *code << OpId{spv::Op::OpPhi, result_type} << operands << EndOp{};
}

Id Module::DeferredOpPhi(Id result_type, std::span<const Id> blocks) {
    deferred_phi_nodes.push_back(code->LocalAddress());
    code->Reserve(3 + blocks.size() * 2);
    *code << OpId{spv::Op::OpPhi, result_type};
    for (const Id block : blocks) {
        *code << u32{0} << block;
    }
    return *code << EndOp{};
}

Id Module::OpLoopMerge(Id merge_block, Id continue_target, spv::LoopControlMask loop_control,
                       std::span<const Id> literals) {
    code->Reserve(4 + literals.size());
    return *code << spv::Op::OpLoopMerge << merge_block << continue_target << loop_control
                 << literals << EndOp{};
}

Id Module::OpSelectionMerge(Id merge_block, spv::SelectionControlMask selection_control) {
    code->Reserve(3);
    return *code << spv::Op::OpSelectionMerge << merge_block << selection_control << EndOp{};
}

Id Module::OpLabel() {
    return Id{++bound};
}

Id Module::OpBranch(Id target_label) {
    code->Reserve(2);
    return *code << spv::Op::OpBranch << target_label << EndOp{};
}

Id Module::OpBranchConditional(Id condition, Id true_label, Id false_label, u32 true_weight,
                               u32 false_weight) {
    code->Reserve(6);
    *code << spv::Op::OpBranchConditional << condition << true_label << false_label;
    if (true_weight != 0 || false_weight != 0) {
        *code << true_weight << false_weight;
    }
    return *code << EndOp{};
}

Id Module::OpSwitch(Id selector, Id default_label, std::span<const Literal> literals,
                    std::span<const Id> labels) {
    assert(literals.size() == labels.size());
    const size_t size = literals.size();
    code->Reserve(3 + size * 2);

    *code << spv::Op::OpSwitch << selector << default_label;
    for (std::size_t i = 0; i < size; ++i) {
        *code << literals[i] << labels[i];
    }
    return *code << EndOp{};
}

void Module::OpReturn() {
    code->Reserve(1);
    *code << spv::Op::OpReturn << EndOp{};
}

void Module::OpUnreachable() {
    code->Reserve(1);
    *code << spv::Op::OpUnreachable << EndOp{};
}

Id Module::OpReturnValue(Id value) {
    code->Reserve(2);
    return *code << spv::Op::OpReturnValue << value << EndOp{};
}

void Module::OpKill() {
    code->Reserve(1);
    *code << spv::Op::OpKill << EndOp{};
}

void Module::OpDemoteToHelperInvocationEXT() {
    code->Reserve(1);
    *code << spv::Op::OpDemoteToHelperInvocationEXT << EndOp{};
}

} // namespace Sirit
