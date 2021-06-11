/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <memory>
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::OpControlBarrier(Id execution, Id memory, Id semantics) {
    auto op = std::make_unique<Op>(spv::Op::OpControlBarrier);
    op->Add(execution);
    op->Add(memory);
    op->Add(semantics);
    return AddCode(std::move(op));
}

Id Module::OpMemoryBarrier(Id scope, Id semantics) {
    auto op = std::make_unique<Op>(spv::Op::OpMemoryBarrier);
    op->Add(scope);
    op->Add(semantics);
    return AddCode(std::move(op));
}

} // namespace Sirit
