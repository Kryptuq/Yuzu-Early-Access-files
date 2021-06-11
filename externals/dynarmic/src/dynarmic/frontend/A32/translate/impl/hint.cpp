/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"
#include "dynarmic/interface/A32/config.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::arm_PLD_imm(bool /*add*/, bool R, Reg /*n*/, Imm<12> /*imm12*/) {
    if (!options.hook_hint_instructions) {
        return true;
    }

    const auto exception = R ? Exception::PreloadData : Exception::PreloadDataWithIntentToWrite;
    return RaiseException(exception);
}

bool TranslatorVisitor::arm_PLD_reg(bool /*add*/, bool R, Reg /*n*/, Imm<5> /*imm5*/, ShiftType /*shift*/, Reg /*m*/) {
    if (!options.hook_hint_instructions) {
        return true;
    }

    const auto exception = R ? Exception::PreloadData : Exception::PreloadDataWithIntentToWrite;
    return RaiseException(exception);
}

bool TranslatorVisitor::arm_SEV() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::SendEvent);
}

bool TranslatorVisitor::arm_SEVL() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::SendEventLocal);
}

bool TranslatorVisitor::arm_WFE() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::WaitForEvent);
}

bool TranslatorVisitor::arm_WFI() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::WaitForInterrupt);
}

bool TranslatorVisitor::arm_YIELD() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::Yield);
}

}  // namespace Dynarmic::A32
