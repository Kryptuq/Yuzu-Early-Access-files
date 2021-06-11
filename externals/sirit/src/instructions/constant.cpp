/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::ConstantTrue(Id result_type) {
    return AddDeclaration(std::make_unique<Op>(spv::Op::OpConstantTrue, bound, result_type));
}

Id Module::ConstantFalse(Id result_type) {
    return AddDeclaration(std::make_unique<Op>(spv::Op::OpConstantFalse, bound, result_type));
}

Id Module::Constant(Id result_type, const Literal& literal) {
    auto op{std::make_unique<Op>(spv::Op::OpConstant, bound, result_type)};
    op->Add(literal);
    return AddDeclaration(std::move(op));
}

Id Module::ConstantComposite(Id result_type, const std::vector<Id>& constituents) {
    auto op{std::make_unique<Op>(spv::Op::OpConstantComposite, bound, result_type)};
    op->Add(constituents);
    return AddDeclaration(std::move(op));
}

Id Module::ConstantSampler(Id result_type, spv::SamplerAddressingMode addressing_mode,
                           bool normalized, spv::SamplerFilterMode filter_mode) {
    auto op{std::make_unique<Op>(spv::Op::OpConstantSampler, bound, result_type)};
    op->Add(static_cast<u32>(addressing_mode));
    op->Add(normalized ? 1 : 0);
    op->Add(static_cast<u32>(filter_mode));
    return AddDeclaration(std::move(op));
}

Id Module::ConstantNull(Id result_type) {
    return AddDeclaration(std::make_unique<Op>(spv::Op::OpConstantNull, bound, result_type));
}

} // namespace Sirit
