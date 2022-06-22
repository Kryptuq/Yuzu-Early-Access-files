/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */
#pragma once

#include <functional>

#include <mcl/stdint.hpp>

namespace Dynarmic {

namespace IR {
class Block;
}  // namespace IR

namespace A64 {

class LocationDescriptor;

using MemoryReadCodeFuncType = std::function<u32(u64 vaddr)>;

struct TranslationOptions {
    /// This changes what IR we emit when we translate an unpredictable instruction.
    /// If this is false, the ExceptionRaised IR instruction is emitted.
    /// If this is true, we define some behaviour for some instructions.
    bool define_unpredictable_behaviour = false;

    /// This tells the translator a wall clock will be used, thus allowing it
    /// to avoid writting certain unnecessary code only needed for cycle timers.
    bool wall_clock_cntpct = false;

    /// This changes what IR we emit when we translate a hint instruction.
    /// If this is false, we treat the instruction as a NOP.
    /// If this is true, we emit an ExceptionRaised instruction.
    bool hook_hint_instructions = true;

    /// This changes what IR we emit when we translate a memory instruction.
    /// If this is false, memory accesses are not considered terminal.
    /// If this is true, memory access are considered terminal. This allows
    /// accurately emulating protection fault handlers.
    bool check_halt_on_memory_access = false;
};

/**
 * This function translates instructions in memory into our intermediate representation.
 * @param descriptor The starting location of the basic block. Includes information like PC, FPCR state, &c.
 * @param memory_read_code The function we should use to read emulated memory.
 * @param options Configures how certain instructions are translated.
 * @return A translated basic block in the intermediate representation.
 */
IR::Block Translate(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code, TranslationOptions options);

/**
 * This function translates a single provided instruction into our intermediate representation.
 * @param block The block to append the IR for the instruction to.
 * @param descriptor The location of the instruction. Includes information like PC, FPCR state, &c.
 * @param instruction The instruction to translate.
 * @return The translated instruction translated to the intermediate representation.
 */
bool TranslateSingleInstruction(IR::Block& block, LocationDescriptor descriptor, u32 instruction);

}  // namespace A64
}  // namespace Dynarmic
