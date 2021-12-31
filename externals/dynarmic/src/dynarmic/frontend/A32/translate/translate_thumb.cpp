/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <tuple>

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/decoder/asimd.h"
#include "dynarmic/frontend/A32/decoder/thumb16.h"
#include "dynarmic/frontend/A32/decoder/thumb32.h"
#include "dynarmic/frontend/A32/decoder/vfp.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/frontend/A32/translate/conditional_state.h"
#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"
#include "dynarmic/frontend/A32/translate/translate_callbacks.h"
#include "dynarmic/frontend/imm.h"
#include "dynarmic/interface/A32/config.h"

namespace Dynarmic::A32 {
namespace {

enum class ThumbInstSize {
    Thumb16,
    Thumb32
};

bool IsThumb16(u16 first_part) {
    return (first_part & 0xF800) < 0xE800;
}

bool IsUnconditionalInstruction(bool is_thumb_16, u32 instruction) {
    if (!is_thumb_16)
        return false;
    if ((instruction & 0xFF00) == 0b10111110'00000000)  // BKPT
        return true;
    if ((instruction & 0xFFC0) == 0b10111010'10000000)  // HLT
        return true;
    return false;
}

std::tuple<u32, ThumbInstSize> ReadThumbInstruction(u32 arm_pc, TranslateCallbacks* tcb) {
    u32 first_part = tcb->MemoryReadCode(arm_pc & 0xFFFFFFFC);
    if ((arm_pc & 0x2) != 0) {
        first_part >>= 16;
    }
    first_part &= 0xFFFF;

    if (IsThumb16(static_cast<u16>(first_part))) {
        // 16-bit thumb instruction
        return std::make_tuple(first_part, ThumbInstSize::Thumb16);
    }

    // 32-bit thumb instruction
    // These always start with 0b11101, 0b11110 or 0b11111.

    u32 second_part = tcb->MemoryReadCode((arm_pc + 2) & 0xFFFFFFFC);
    if (((arm_pc + 2) & 0x2) != 0) {
        second_part >>= 16;
    }
    second_part &= 0xFFFF;

    return std::make_tuple(static_cast<u32>((first_part << 16) | second_part), ThumbInstSize::Thumb32);
}

// Convert from thumb ASIMD format to ARM ASIMD format.
u32 ConvertASIMDInstruction(u32 thumb_instruction) {
    if ((thumb_instruction & 0xEF000000) == 0xEF000000) {
        const bool U = Common::Bit<28>(thumb_instruction);
        return 0xF2000000 | (U << 24) | (thumb_instruction & 0x00FFFFFF);
    }

    if ((thumb_instruction & 0xFF000000) == 0xF9000000) {
        return 0xF4000000 | (thumb_instruction & 0x00FFFFFF);
    }

    return 0xF7F0A000;  // UDF
}

bool MaybeVFPOrASIMDInstruction(u32 thumb_instruction) {
    return (thumb_instruction & 0xEC000000) == 0xEC000000 || (thumb_instruction & 0xFF100000) == 0xF9000000;
}

}  // namespace

IR::Block TranslateThumb(LocationDescriptor descriptor, TranslateCallbacks* tcb, const TranslationOptions& options) {
    const bool single_step = descriptor.SingleStepping();

    IR::Block block{descriptor};
    TranslatorVisitor visitor{block, descriptor, options};

    bool should_continue = true;
    do {
        const u32 arm_pc = visitor.ir.current_location.PC();
        const auto [thumb_instruction, inst_size] = ReadThumbInstruction(arm_pc, tcb);
        const bool is_thumb_16 = inst_size == ThumbInstSize::Thumb16;
        visitor.current_instruction_size = is_thumb_16 ? 2 : 4;

        tcb->PreCodeTranslationHook(false, arm_pc, visitor.ir);

        if (IsUnconditionalInstruction(is_thumb_16, thumb_instruction) || visitor.ThumbConditionPassed()) {
            if (is_thumb_16) {
                if (const auto decoder = DecodeThumb16<TranslatorVisitor>(static_cast<u16>(thumb_instruction))) {
                    should_continue = decoder->get().call(visitor, static_cast<u16>(thumb_instruction));
                } else {
                    should_continue = visitor.thumb16_UDF();
                }
            } else {
                if (MaybeVFPOrASIMDInstruction(thumb_instruction)) {
                    if (const auto vfp_decoder = DecodeVFP<TranslatorVisitor>(thumb_instruction)) {
                        should_continue = vfp_decoder->get().call(visitor, thumb_instruction);
                    } else if (const auto asimd_decoder = DecodeASIMD<TranslatorVisitor>(ConvertASIMDInstruction(thumb_instruction))) {
                        should_continue = asimd_decoder->get().call(visitor, ConvertASIMDInstruction(thumb_instruction));
                    } else if (const auto decoder = DecodeThumb32<TranslatorVisitor>(thumb_instruction)) {
                        should_continue = decoder->get().call(visitor, thumb_instruction);
                    } else {
                        should_continue = visitor.thumb32_UDF();
                    }
                } else if (const auto decoder = DecodeThumb32<TranslatorVisitor>(thumb_instruction)) {
                    should_continue = decoder->get().call(visitor, thumb_instruction);
                } else {
                    should_continue = visitor.thumb32_UDF();
                }
            }
        }

        if (visitor.cond_state == ConditionalState::Break) {
            break;
        }

        visitor.ir.current_location = visitor.ir.current_location.AdvancePC(is_thumb_16 ? 2 : 4).AdvanceIT();
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

bool TranslateSingleThumbInstruction(IR::Block& block, LocationDescriptor descriptor, u32 thumb_instruction) {
    TranslatorVisitor visitor{block, descriptor, {}};

    bool should_continue = true;

    const bool is_thumb_16 = IsThumb16(static_cast<u16>(thumb_instruction));
    visitor.current_instruction_size = is_thumb_16 ? 2 : 4;

    if (is_thumb_16) {
        if (const auto decoder = DecodeThumb16<TranslatorVisitor>(static_cast<u16>(thumb_instruction))) {
            should_continue = decoder->get().call(visitor, static_cast<u16>(thumb_instruction));
        } else {
            should_continue = visitor.thumb16_UDF();
        }
    } else {
        thumb_instruction = Common::SwapHalves32(thumb_instruction);
        if (MaybeVFPOrASIMDInstruction(thumb_instruction)) {
            if (const auto vfp_decoder = DecodeVFP<TranslatorVisitor>(thumb_instruction)) {
                should_continue = vfp_decoder->get().call(visitor, thumb_instruction);
            } else if (const auto asimd_decoder = DecodeASIMD<TranslatorVisitor>(ConvertASIMDInstruction(thumb_instruction))) {
                should_continue = asimd_decoder->get().call(visitor, ConvertASIMDInstruction(thumb_instruction));
            } else if (const auto decoder = DecodeThumb32<TranslatorVisitor>(thumb_instruction)) {
                should_continue = decoder->get().call(visitor, thumb_instruction);
            } else {
                should_continue = visitor.thumb32_UDF();
            }
        } else if (const auto decoder = DecodeThumb32<TranslatorVisitor>(thumb_instruction)) {
            should_continue = decoder->get().call(visitor, thumb_instruction);
        } else {
            should_continue = visitor.thumb32_UDF();
        }
    }

    const s32 advance_pc = is_thumb_16 ? 2 : 4;
    visitor.ir.current_location = visitor.ir.current_location.AdvancePC(advance_pc);
    block.CycleCount()++;

    block.SetEndLocation(visitor.ir.current_location);

    return should_continue;
}

}  // namespace Dynarmic::A32
