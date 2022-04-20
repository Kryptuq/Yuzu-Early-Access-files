/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */
#pragma once

#include <mcl/stdint.hpp>

namespace Dynarmic::A32 {

using VAddr = u32;

class IREmitter;

struct TranslateCallbacks {
    // All reads through this callback are 4-byte aligned.
    // Memory must be interpreted as little endian.
    virtual std::uint32_t MemoryReadCode(VAddr vaddr) = 0;

    // Thus function is called before the instruction at pc is interpreted.
    // IR code can be emitted by the callee prior to translation of the instruction.
    virtual void PreCodeTranslationHook(bool is_thumb, VAddr pc, A32::IREmitter& ir) = 0;
};

}  // namespace Dynarmic::A32
