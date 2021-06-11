/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/translate.h"

#include "dynarmic/frontend/A64/decoder/a64.h"
#include "dynarmic/frontend/A64/location_descriptor.h"
#include "dynarmic/frontend/A64/translate/impl/impl.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/terminal.h"

namespace Dynarmic::A64 {

IR::Block Translate(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code, TranslationOptions options) {
    const bool single_step = descriptor.SingleStepping();

    IR::Block block{descriptor};
    TranslatorVisitor visitor{block, descriptor, std::move(options)};

    bool should_continue = true;
    do {
        const u64 pc = visitor.ir.current_location->PC();
        const u32 instruction = memory_read_code(pc);

        if (auto decoder = Decode<TranslatorVisitor>(instruction)) {
            should_continue = decoder->get().call(visitor, instruction);
        } else {
            should_continue = visitor.InterpretThisInstruction();
        }

        visitor.ir.current_location = visitor.ir.current_location->AdvancePC(4);
        block.CycleCount()++;
    } while (should_continue && !single_step);

    if (single_step && should_continue) {
        visitor.ir.SetTerm(IR::Term::LinkBlock{*visitor.ir.current_location});
    }

    ASSERT_MSG(block.HasTerminal(), "Terminal has not been set");

    block.SetEndLocation(*visitor.ir.current_location);

    return block;
}

bool TranslateSingleInstruction(IR::Block& block, LocationDescriptor descriptor, u32 instruction) {
    TranslatorVisitor visitor{block, descriptor, {}};

    bool should_continue = true;
    if (auto decoder = Decode<TranslatorVisitor>(instruction)) {
        should_continue = decoder->get().call(visitor, instruction);
    } else {
        should_continue = visitor.InterpretThisInstruction();
    }

    visitor.ir.current_location = visitor.ir.current_location->AdvancePC(4);
    block.CycleCount()++;

    block.SetEndLocation(*visitor.ir.current_location);

    return should_continue;
}

}  // namespace Dynarmic::A64
