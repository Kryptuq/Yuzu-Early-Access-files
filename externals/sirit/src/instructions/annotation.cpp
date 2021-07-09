/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <span>

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::Decorate(Id target, spv::Decoration decoration, std::span<const Literal> literals) {
    annotations->Reserve(3 + literals.size());
    return *annotations << spv::Op::OpDecorate << target << decoration << literals << EndOp{};
}

Id Module::MemberDecorate(Id structure_type, Literal member, spv::Decoration decoration,
                          std::span<const Literal> literals) {
    annotations->Reserve(4 + literals.size());
    return *annotations << spv::Op::OpMemberDecorate << structure_type << member << decoration
                        << literals << EndOp{};
}

} // namespace Sirit
