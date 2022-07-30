/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <string>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <mcl/bit/swap.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/llvm_disassemble.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/decoder/arm.h"
#include "dynarmic/frontend/A32/decoder/asimd.h"
#include "dynarmic/frontend/A32/decoder/vfp.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"
#include "dynarmic/frontend/A64/a64_location_descriptor.h"
#include "dynarmic/frontend/A64/decoder/a64.h"
#include "dynarmic/frontend/A64/translate/a64_translate.h"
#include "dynarmic/frontend/A64/translate/impl/impl.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A32/disassembler.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/opt/passes.h"

using namespace Dynarmic;

const char* GetNameOfA32Instruction(u32 instruction) {
    if (auto vfp_decoder = A32::DecodeVFP<A32::TranslatorVisitor>(instruction)) {
        return vfp_decoder->get().GetName();
    } else if (auto asimd_decoder = A32::DecodeASIMD<A32::TranslatorVisitor>(instruction)) {
        return asimd_decoder->get().GetName();
    } else if (auto decoder = A32::DecodeArm<A32::TranslatorVisitor>(instruction)) {
        return decoder->get().GetName();
    }
    return "<null>";
}

const char* GetNameOfA64Instruction(u32 instruction) {
    if (auto decoder = A64::Decode<A64::TranslatorVisitor>(instruction)) {
        return decoder->get().GetName();
    }
    return "<null>";
}

void PrintA32Instruction(u32 instruction) {
    fmt::print("{:08x} {}\n", instruction, Common::DisassembleAArch32(false, 0, (u8*)&instruction, sizeof(instruction)));
    fmt::print("Name: {}\n", GetNameOfA32Instruction(instruction));

    const A32::LocationDescriptor location{0, {}, {}};
    IR::Block block{location};
    const bool should_continue = A32::TranslateSingleInstruction(block, location, instruction);
    fmt::print("should_continue: {}\n\n", should_continue);
    fmt::print("IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));

    Optimization::A32GetSetElimination(block, {});
    Optimization::DeadCodeElimination(block);
    Optimization::ConstantPropagation(block);
    Optimization::DeadCodeElimination(block);
    Optimization::IdentityRemovalPass(block);

    fmt::print("Optimized IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));
}

void PrintA64Instruction(u32 instruction) {
    fmt::print("{:08x} {}\n", instruction, Common::DisassembleAArch64(instruction));
    fmt::print("Name: {}\n", GetNameOfA64Instruction(instruction));

    const A64::LocationDescriptor location{0, {}};
    IR::Block block{location};
    const bool should_continue = A64::TranslateSingleInstruction(block, location, instruction);
    fmt::print("should_continue: {}\n\n", should_continue);
    fmt::print("IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));

    Optimization::A64GetSetElimination(block);
    Optimization::DeadCodeElimination(block);
    Optimization::ConstantPropagation(block);
    Optimization::DeadCodeElimination(block);
    Optimization::IdentityRemovalPass(block);

    fmt::print("Optimized IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));
}

void PrintThumbInstruction(u32 instruction) {
    const size_t inst_size = (instruction >> 16) == 0 ? 2 : 4;
    if (inst_size == 4)
        instruction = mcl::bit::swap_halves_32(instruction);

    fmt::print("{:08x} {}\n", instruction, Common::DisassembleAArch32(true, 0, (u8*)&instruction, inst_size));

    const A32::LocationDescriptor location{0, A32::PSR{0x1F0}, {}};
    IR::Block block{location};
    const bool should_continue = A32::TranslateSingleInstruction(block, location, instruction);
    fmt::print("should_continue: {}\n\n", should_continue);
    fmt::print("IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));

    Optimization::A32GetSetElimination(block, {});
    Optimization::DeadCodeElimination(block);
    Optimization::ConstantPropagation(block);
    Optimization::DeadCodeElimination(block);
    Optimization::IdentityRemovalPass(block);

    fmt::print("Optimized IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));
}

class ExecEnv final : public Dynarmic::A32::UserCallbacks {
public:
    u64 ticks_left = 0;
    std::map<u32, u8> memory;

    std::uint8_t MemoryRead8(u32 vaddr) override {
        if (auto iter = memory.find(vaddr); iter != memory.end()) {
            return iter->second;
        }
        return 0;
    }
    std::uint16_t MemoryRead16(u32 vaddr) override {
        return u16(MemoryRead8(vaddr)) | u16(MemoryRead8(vaddr + 1)) << 8;
    }
    std::uint32_t MemoryRead32(u32 vaddr) override {
        return u32(MemoryRead16(vaddr)) | u32(MemoryRead16(vaddr + 2)) << 16;
    }
    std::uint64_t MemoryRead64(u32 vaddr) override {
        return u64(MemoryRead32(vaddr)) | u64(MemoryRead32(vaddr + 4)) << 32;
    }

    void MemoryWrite8(u32 vaddr, std::uint8_t value) override {
        memory[vaddr] = value;
    }
    void MemoryWrite16(u32 vaddr, std::uint16_t value) override {
        MemoryWrite8(vaddr, static_cast<u8>(value));
        MemoryWrite8(vaddr + 1, static_cast<u8>(value >> 8));
    }
    void MemoryWrite32(u32 vaddr, std::uint32_t value) override {
        MemoryWrite16(vaddr, static_cast<u16>(value));
        MemoryWrite16(vaddr + 2, static_cast<u16>(value >> 16));
    }
    void MemoryWrite64(u32 vaddr, std::uint64_t value) override {
        MemoryWrite32(vaddr, static_cast<u32>(value));
        MemoryWrite32(vaddr + 4, static_cast<u32>(value >> 32));
    }

    void InterpreterFallback(u32 pc, size_t num_instructions) override {
        fmt::print("> InterpreterFallback({:08x}, {}) code = {:08x}\n", pc, num_instructions, *MemoryReadCode(pc));
    }
    void CallSVC(std::uint32_t swi) override {
        fmt::print("> CallSVC({})\n", swi);
    }
    void ExceptionRaised(u32 pc, Dynarmic::A32::Exception exception) override {
        fmt::print("> ExceptionRaised({:08x}, {})", pc, static_cast<size_t>(exception));
    }

    void AddTicks(std::uint64_t ticks) override {
        if (ticks > ticks_left) {
            ticks_left = 0;
            return;
        }
        ticks_left -= ticks;
    }
    std::uint64_t GetTicksRemaining() override {
        return ticks_left;
    }
};

void ExecuteA32Instruction(u32 instruction) {
    ExecEnv env;
    A32::Jit cpu{A32::UserConfig{&env}};
    env.ticks_left = 1;

    std::array<u32, 16> regs{};
    std::array<u32, 64> ext_regs{};
    u32 cpsr = 0;
    u32 fpscr = 0;

    const std::map<std::string, u32*> name_map = [&regs, &ext_regs, &cpsr, &fpscr] {
        std::map<std::string, u32*> name_map;
        for (size_t i = 0; i < regs.size(); i++) {
            name_map[fmt::format("r{}", i)] = &regs[i];
        }
        for (size_t i = 0; i < ext_regs.size(); i++) {
            name_map[fmt::format("s{}", i)] = &ext_regs[i];
        }
        name_map["sp"] = &regs[13];
        name_map["lr"] = &regs[14];
        name_map["pc"] = &regs[15];
        name_map["cpsr"] = &cpsr;
        name_map["fpscr"] = &fpscr;
        return name_map;
    }();

    const auto get_line = []() {
        std::string line;
        std::getline(std::cin, line);
        std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return line;
    };

    const auto get_value = [&get_line]() -> std::optional<u32> {
        std::string line = get_line();
        if (line.length() > 2 && line[0] == '0' && line[1] == 'x')
            line = line.substr(2);
        if (line.length() > 8)
            return std::nullopt;

        char* endptr;
        const u32 value = strtol(line.c_str(), &endptr, 16);
        if (line.c_str() + line.length() != endptr)
            return std::nullopt;

        return value;
    };

    while (std::cin) {
        fmt::print("register: ");
        const std::string reg_name = get_line();
        if (const auto iter = name_map.find(reg_name); iter != name_map.end()) {
            fmt::print("value: ");
            if (const auto value = get_value()) {
                *(iter->second) = *value;
                fmt::print("> {} = 0x{:08x}\n", reg_name, *value);
            }
        } else if (reg_name == "mem" || reg_name == "memory") {
            fmt::print("address: ");
            if (const auto address = get_value()) {
                fmt::print("value: ");
                if (const auto value = get_value()) {
                    env.MemoryWrite32(*address, *value);
                    fmt::print("> mem[0x{:08x}] = 0x{:08x}\n", *address, *value);
                }
            }
        } else if (reg_name == "end") {
            break;
        }
    }
    fmt::print("\n\n");

    cpu.Regs() = regs;
    cpu.ExtRegs() = ext_regs;
    cpu.SetCpsr(cpsr);
    cpu.SetFpscr(fpscr);

    const u32 initial_pc = regs[15];
    env.MemoryWrite32(initial_pc + 0, instruction);
    env.MemoryWrite32(initial_pc + 4, 0xEAFFFFFE);  // B +0

    cpu.Run();

    fmt::print("Registers modified:\n");
    for (size_t i = 0; i < regs.size(); ++i) {
        if (regs[i] != cpu.Regs()[i]) {
            fmt::print("{:3s}: {:08x}\n", static_cast<A32::Reg>(i), cpu.Regs()[i]);
        }
    }
    for (size_t i = 0; i < ext_regs.size(); ++i) {
        if (ext_regs[i] != cpu.ExtRegs()[i]) {
            fmt::print("{:3s}: {:08x}\n", static_cast<A32::ExtReg>(i), cpu.Regs()[i]);
        }
    }
    if (cpsr != cpu.Cpsr()) {
        fmt::print("cpsr {:08x}\n", cpu.Cpsr());
    }
    if (fpscr != cpu.Fpscr()) {
        fmt::print("fpscr{:08x}\n", cpu.Fpscr());
    }
    fmt::print("Modified memory:\n");
    for (auto iter = env.memory.begin(); iter != env.memory.end(); ++iter) {
        fmt::print("{:08x} {:02x}\n", iter->first, iter->second);
    }
}

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        fmt::print("usage: {} <a32/a64/thumb> <instruction_in_hex> [-exec]\n", argv[0]);
        return 1;
    }

    const char* const hex_instruction = [argv] {
        if (strlen(argv[2]) > 2 && argv[2][0] == '0' && argv[2][1] == 'x') {
            return argv[2] + 2;
        }
        return argv[2];
    }();

    if (strlen(hex_instruction) > 8) {
        fmt::print("hex string too long\n");
        return 1;
    }

    const u32 instruction = strtol(hex_instruction, nullptr, 16);

    if (strcmp(argv[1], "a32") == 0) {
        PrintA32Instruction(instruction);
    } else if (strcmp(argv[1], "a64") == 0) {
        PrintA64Instruction(instruction);
    } else if (strcmp(argv[1], "t32") == 0 || strcmp(argv[1], "t16") == 0 || strcmp(argv[1], "thumb") == 0) {
        PrintThumbInstruction(instruction);
    } else {
        fmt::print("Invalid mode: {}\nValid values: a32, a64, thumb\n", argv[1]);
        return 1;
    }

    if (argc == 4) {
        if (strcmp(argv[3], "-exec") != 0) {
            fmt::print("Invalid option {}\n", argv[3]);
            return 1;
        }

        if (strcmp(argv[1], "a32") == 0) {
            ExecuteA32Instruction(instruction);
        } else {
            fmt::print("Executing in this mode not currently supported\n");
            return 1;
        }
    }

    return 0;
}
