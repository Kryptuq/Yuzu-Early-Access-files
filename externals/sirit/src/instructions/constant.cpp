/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::ConstantTrue(Id result_type) {
    declarations->Reserve(3);
    return *declarations << OpId{spv::Op::OpConstantTrue, result_type} << EndOp{};
}

Id Module::ConstantFalse(Id result_type) {
    declarations->Reserve(3);
    return *declarations << OpId{spv::Op::OpConstantFalse, result_type} << EndOp{};
}

Id Module::Constant(Id result_type, const Literal& literal) {
    declarations->Reserve(3 + 2);
    return *declarations << OpId{spv::Op::OpConstant, result_type} << literal << EndOp{};
}

Id Module::ConstantComposite(Id result_type, std::span<const Id> constituents) {
    declarations->Reserve(3 + constituents.size());
    return *declarations << OpId{spv::Op::OpConstantComposite, result_type} << constituents
                         << EndOp{};
}

Id Module::ConstantSampler(Id result_type, spv::SamplerAddressingMode addressing_mode,
                           bool normalized, spv::SamplerFilterMode filter_mode) {
    declarations->Reserve(6);
    return *declarations << OpId{spv::Op::OpConstantSampler, result_type} << addressing_mode
                         << normalized << filter_mode << EndOp{};
}

Id Module::ConstantNull(Id result_type) {
    declarations->Reserve(3);
    return *declarations << OpId{spv::Op::OpConstantNull, result_type} << EndOp{};
}

} // namespace Sirit
