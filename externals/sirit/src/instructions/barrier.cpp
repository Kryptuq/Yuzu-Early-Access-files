/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::OpControlBarrier(Id execution, Id memory, Id semantics) {
    code->Reserve(4);
    return *code << spv::Op::OpControlBarrier << execution << memory << semantics << EndOp{};
}

Id Module::OpMemoryBarrier(Id scope, Id semantics) {
    code->Reserve(3);
    return *code << spv::Op::OpMemoryBarrier << scope << semantics << EndOp{};
}

} // namespace Sirit
