/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/assert.h"
#include "dynarmic/frontend/A32/decoder/arm.h"
#include "dynarmic/frontend/A32/decoder/asimd.h"
#include "dynarmic/frontend/A32/decoder/vfp.h"
#include "dynarmic/frontend/A32/location_descriptor.h"
#include "dynarmic/frontend/A32/translate/conditional_state.h"
#include "dynarmic/frontend/A32/translate/impl/translate.h"
#include "dynarmic/frontend/A32/translate/translate.h"
#include "dynarmic/frontend/A32/translate/translate_callbacks.h"
#include "dynarmic/frontend/A32/types.h"
#include "dynarmic/interface/A32/config.h"
#include "dynarmic/ir/basic_block.h"

namespace Dynarmic::A32 {

IR::Block TranslateArm(LocationDescriptor descriptor, TranslateCallbacks* tcb, const TranslationOptions& options) {
    const bool single_step = descriptor.SingleStepping();

    IR::Block block{descriptor};
    TranslatorVisitor visitor{block, descriptor, options};

    bool should_continue = true;
    do {
        const u32 arm_pc = visitor.ir.current_location.PC();
        const u32 arm_instruction = tcb->MemoryReadCode(arm_pc);
        visitor.current_instruction_size = 4;

        tcb->PreCodeTranslationHook(false, arm_pc, visitor.ir);

        if (const auto vfp_decoder = DecodeVFP<TranslatorVisitor>(arm_instruction)) {
            should_continue = vfp_decoder->get().call(visitor, arm_instruction);
        } else if (const auto asimd_decoder = DecodeASIMD<TranslatorVisitor>(arm_instruction)) {
            should_continue = asimd_decoder->get().call(visitor, arm_instruction);
        } else if (const auto decoder = DecodeArm<TranslatorVisitor>(arm_instruction)) {
            should_continue = decoder->get().call(visitor, arm_instruction);
        } else {
            should_continue = visitor.arm_UDF();
        }

        if (visitor.cond_state == ConditionalState::Break) {
            break;
        }

        visitor.ir.current_location = visitor.ir.current_location.AdvancePC(4);
        block.CycleCount()++;
    } while (should_continue && CondCanContinue(visitor.cond_state, visitor.ir) && !single_step);

    if (visitor.cond_state == ConditionalState::Translating || visitor.cond_state == ConditionalState::Trailing || single_step) {
        if (should_continue) {
            if (single_step) {
                visitor.ir.SetTerm(IR::Term::LinkBlock{visitor.ir.current_location});
            } else {
                visitor.ir.SetTerm(IR::Term::LinkBlockFast{visitor.ir.current_location});
            }
        }
    }

    ASSERT_MSG(block.HasTerminal(), "Terminal has not been set");

    block.SetEndLocation(visitor.ir.current_location);

    return block;
}

bool TranslateSingleArmInstruction(IR::Block& block, LocationDescriptor descriptor, u32 arm_instruction) {
    TranslatorVisitor visitor{block, descriptor, {}};

    // TODO: Proper cond handling

    bool should_continue = true;
    if (const auto vfp_decoder = DecodeVFP<TranslatorVisitor>(arm_instruction)) {
        should_continue = vfp_decoder->get().call(visitor, arm_instruction);
    } else if (const auto asimd_decoder = DecodeASIMD<TranslatorVisitor>(arm_instruction)) {
        should_continue = asimd_decoder->get().call(visitor, arm_instruction);
    } else if (const auto decoder = DecodeArm<TranslatorVisitor>(arm_instruction)) {
        should_continue = decoder->get().call(visitor, arm_instruction);
    } else {
        should_continue = visitor.arm_UDF();
    }

    // TODO: Feedback resulting cond status to caller somehow.

    visitor.ir.current_location = visitor.ir.current_location.AdvancePC(4);
    block.CycleCount()++;

    block.SetEndLocation(visitor.ir.current_location);

    return should_continue;
}

}  // namespace Dynarmic::A32
