/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/conditional_state.h"

#include <algorithm>

#include "dynarmic/common/assert.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/A32/ir_emitter.h"
#include "dynarmic/frontend/A32/translate/impl/translate.h"
#include "dynarmic/interface/A32/config.h"
#include "dynarmic/ir/cond.h"

namespace Dynarmic::A32 {

bool CondCanContinue(ConditionalState cond_state, const A32::IREmitter& ir) {
    ASSERT_MSG(cond_state != ConditionalState::Break, "Should never happen.");

    if (cond_state == ConditionalState::None)
        return true;

    // TODO: This is more conservative than necessary.
    return std::all_of(ir.block.begin(), ir.block.end(), [](const IR::Inst& inst) { return !inst.WritesToCPSR(); });
}

bool IsConditionPassed(TranslatorVisitor& v, IR::Cond cond) {
    ASSERT_MSG(v.cond_state != ConditionalState::Break,
               "This should never happen. We requested a break but that wasn't honored.");

    if (cond == IR::Cond::NV) {
        // NV conditional is obsolete
        v.RaiseException(Exception::UnpredictableInstruction);
        return false;
    }

    if (v.cond_state == ConditionalState::Translating) {
        if (v.ir.block.ConditionFailedLocation() != v.ir.current_location || cond == IR::Cond::AL) {
            v.cond_state = ConditionalState::Trailing;
        } else {
            if (cond == v.ir.block.GetCondition()) {
                v.ir.block.SetConditionFailedLocation(v.ir.current_location.AdvancePC(static_cast<int>(v.current_instruction_size)).AdvanceIT());
                v.ir.block.ConditionFailedCycleCount()++;
                return true;
            }

            // cond has changed, abort
            v.cond_state = ConditionalState::Break;
            v.ir.SetTerm(IR::Term::LinkBlockFast{v.ir.current_location});
            return false;
        }
    }

    if (cond == IR::Cond::AL) {
        // Everything is fine with the world
        return true;
    }

    // non-AL cond

    if (!v.ir.block.empty()) {
        // We've already emitted instructions. Quit for now, we'll make a new block here later.
        v.cond_state = ConditionalState::Break;
        v.ir.SetTerm(IR::Term::LinkBlockFast{v.ir.current_location});
        return false;
    }

    // We've not emitted instructions yet.
    // We'll emit one instruction, and set the block-entry conditional appropriately.

    v.cond_state = ConditionalState::Translating;
    v.ir.block.SetCondition(cond);
    v.ir.block.SetConditionFailedLocation(v.ir.current_location.AdvancePC(static_cast<int>(v.current_instruction_size)).AdvanceIT());
    v.ir.block.ConditionFailedCycleCount() = v.ir.block.CycleCount() + 1;
    return true;
}

}  // namespace Dynarmic::A32
