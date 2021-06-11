/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <memory>
#include <vector>
#include "common_types.h"
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::Decorate(Id target, spv::Decoration decoration, const std::vector<Literal>& literals) {
    auto op{std::make_unique<Op>(spv::Op::OpDecorate)};
    op->Add(target);
    op->Add(static_cast<u32>(decoration));
    op->Add(literals);
    AddAnnotation(std::move(op));
    return target;
}

Id Module::MemberDecorate(Id structure_type, Literal member, spv::Decoration decoration,
                          const std::vector<Literal>& literals) {
    auto op{std::make_unique<Op>(spv::Op::OpMemberDecorate)};
    op->Add(structure_type);
    op->Add(member);
    op->Add(static_cast<u32>(decoration));
    op->Add(literals);
    AddAnnotation(std::move(op));
    return structure_type;
}

} // namespace Sirit
