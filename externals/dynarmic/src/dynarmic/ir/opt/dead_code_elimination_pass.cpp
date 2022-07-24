/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <mcl/iterator/reverse.hpp>

#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Optimization {

void DeadCodeElimination(IR::Block& block) {
    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.
    for (auto& inst : mcl::iterator::reverse(block)) {
        if (!inst.HasUses() && !inst.MayHaveSideEffects()) {
            inst.Invalidate();
        }
    }
}

}  // namespace Dynarmic::Optimization
