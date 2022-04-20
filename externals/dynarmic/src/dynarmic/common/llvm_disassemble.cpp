/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <string>

#include <fmt/format.h>

#ifdef DYNARMIC_USE_LLVM
#    include <llvm-c/Disassembler.h>
#    include <llvm-c/Target.h>
#endif

#include <mcl/assert.hpp>
#include <mcl/bit_cast.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/llvm_disassemble.h"

namespace Dynarmic::Common {

std::string DisassembleX64(const void* begin, const void* end) {
    std::string result;

#ifdef DYNARMIC_USE_LLVM
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86Disassembler();
    LLVMDisasmContextRef llvm_ctx = LLVMCreateDisasm("x86_64", nullptr, 0, nullptr, nullptr);
    LLVMSetDisasmOptions(llvm_ctx, LLVMDisassembler_Option_AsmPrinterVariant);

    const u8* pos = reinterpret_cast<const u8*>(begin);
    size_t remaining = reinterpret_cast<size_t>(end) - reinterpret_cast<size_t>(pos);
    while (pos < end) {
        char buffer[80];
        size_t inst_size = LLVMDisasmInstruction(llvm_ctx, const_cast<u8*>(pos), remaining, reinterpret_cast<u64>(pos), buffer, sizeof(buffer));
        ASSERT(inst_size);
        for (const u8* i = pos; i < pos + inst_size; i++)
            result += fmt::format("{:02x} ", *i);
        for (size_t i = inst_size; i < 10; i++)
            result += "   ";
        result += buffer;
        result += '\n';

        pos += inst_size;
        remaining -= inst_size;
    }

    LLVMDisasmDispose(llvm_ctx);
#else
    result += fmt::format("(recompile with DYNARMIC_USE_LLVM=ON to disassemble the generated x86_64 code)\n");
    result += fmt::format("start: {:016x}, end: {:016x}\n", mcl::bit_cast<u64>(begin), mcl::bit_cast<u64>(end));
#endif

    return result;
}

std::string DisassembleAArch32([[maybe_unused]] bool is_thumb, [[maybe_unused]] u32 pc, [[maybe_unused]] const u8* instructions, [[maybe_unused]] size_t length) {
    std::string result;

#ifdef DYNARMIC_USE_LLVM
    LLVMInitializeARMTargetInfo();
    LLVMInitializeARMTargetMC();
    LLVMInitializeARMDisassembler();
    LLVMDisasmContextRef llvm_ctx = LLVMCreateDisasm(is_thumb ? "thumbv8-arm" : "armv8-arm", nullptr, 0, nullptr, nullptr);
    LLVMSetDisasmOptions(llvm_ctx, LLVMDisassembler_Option_AsmPrinterVariant);

    char buffer[1024];
    while (length) {
        size_t inst_size = LLVMDisasmInstruction(llvm_ctx, const_cast<u8*>(instructions), length, pc, buffer, sizeof(buffer));

        result += fmt::format("{:08x}    ", pc);
        for (size_t i = 0; i < 4; i++) {
            if (i < inst_size) {
                result += fmt::format("{:02x}", instructions[inst_size - i - 1]);
            } else {
                result += "  ";
            }
        }
        result += inst_size > 0 ? buffer : "<invalid instruction>";
        result += '\n';

        if (inst_size == 0)
            inst_size = is_thumb ? 2 : 4;
        if (length <= inst_size)
            break;

        pc += inst_size;
        instructions += inst_size;
        length -= inst_size;
    }

    LLVMDisasmDispose(llvm_ctx);
#else
    result += fmt::format("(disassembly disabled)\n");
#endif

    return result;
}

std::string DisassembleAArch64([[maybe_unused]] u32 instruction, [[maybe_unused]] u64 pc) {
    std::string result;

#ifdef DYNARMIC_USE_LLVM
    LLVMInitializeAArch64TargetInfo();
    LLVMInitializeAArch64TargetMC();
    LLVMInitializeAArch64Disassembler();
    LLVMDisasmContextRef llvm_ctx = LLVMCreateDisasm("aarch64", nullptr, 0, nullptr, nullptr);
    LLVMSetDisasmOptions(llvm_ctx, LLVMDisassembler_Option_AsmPrinterVariant);

    char buffer[80];
    size_t inst_size = LLVMDisasmInstruction(llvm_ctx, (u8*)&instruction, sizeof(instruction), pc, buffer, sizeof(buffer));
    result = inst_size > 0 ? buffer : "<invalid instruction>";
    result += '\n';

    LLVMDisasmDispose(llvm_ctx);
#else
    result += fmt::format("(disassembly disabled)\n");
#endif

    return result;
}

}  // namespace Dynarmic::Common
