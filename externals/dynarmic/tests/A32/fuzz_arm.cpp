/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>
#include <array>
#include <cstdio>
#include <functional>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "../fuzz_util.h"
#include "../rand_int.h"
#include "../unicorn_emu/a32_unicorn.h"
#include "./testenv.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/llvm_disassemble.h"
#include "dynarmic/common/scope_exit.h"
#include "dynarmic/frontend/A32/ITState.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/location_descriptor.h"
#include "dynarmic/ir/opcodes.h"

// Must be declared last for all necessary operator<< to be declared prior to this.
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace {
using namespace Dynarmic;

bool ShouldTestInst(u32 instruction, u32 pc, bool is_thumb, bool is_last_inst, A32::ITState it_state = {}) {
    const A32::LocationDescriptor location = A32::LocationDescriptor{pc, {}, {}}.SetTFlag(is_thumb).SetIT(it_state);
    IR::Block block{location};
    const bool should_continue = A32::TranslateSingleInstruction(block, location, instruction);

    if (!should_continue && !is_last_inst) {
        return false;
    }

    if (auto terminal = block.GetTerminal(); boost::get<IR::Term::Interpret>(&terminal)) {
        return false;
    }

    for (const auto& ir_inst : block) {
        switch (ir_inst.GetOpcode()) {
        case IR::Opcode::A32ExceptionRaised:
        case IR::Opcode::A32CallSupervisor:
        case IR::Opcode::A32CoprocInternalOperation:
        case IR::Opcode::A32CoprocSendOneWord:
        case IR::Opcode::A32CoprocSendTwoWords:
        case IR::Opcode::A32CoprocGetOneWord:
        case IR::Opcode::A32CoprocGetTwoWords:
        case IR::Opcode::A32CoprocLoadWords:
        case IR::Opcode::A32CoprocStoreWords:
            return false;
        // Currently unimplemented in Unicorn
        case IR::Opcode::FPVectorRecipEstimate16:
        case IR::Opcode::FPVectorRSqrtEstimate16:
        case IR::Opcode::VectorPolynomialMultiplyLong64:
            return false;
        default:
            continue;
        }
    }

    return true;
}

u32 GenRandomArmInst(u32 pc, bool is_last_inst) {
    static const struct InstructionGeneratorInfo {
        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;
    } instructions = [] {
        const std::vector<std::tuple<std::string, const char*>> list{
#define INST(fn, name, bitstring) {#fn, bitstring},
#include "dynarmic/frontend/A32/decoder/arm.inc"
#include "dynarmic/frontend/A32/decoder/asimd.inc"
#include "dynarmic/frontend/A32/decoder/vfp.inc"
#undef INST
        };

        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;

        // List of instructions not to test
        static constexpr std::array do_not_test{
            // Translating load/stores
            "arm_LDRBT", "arm_LDRBT", "arm_LDRHT", "arm_LDRHT", "arm_LDRSBT", "arm_LDRSBT", "arm_LDRSHT", "arm_LDRSHT", "arm_LDRT", "arm_LDRT",
            "arm_STRBT", "arm_STRBT", "arm_STRHT", "arm_STRHT", "arm_STRT", "arm_STRT",
            // Exclusive load/stores
            "arm_LDREXB", "arm_LDREXD", "arm_LDREXH", "arm_LDREX", "arm_LDAEXB", "arm_LDAEXD", "arm_LDAEXH", "arm_LDAEX",
            "arm_STREXB", "arm_STREXD", "arm_STREXH", "arm_STREX", "arm_STLEXB", "arm_STLEXD", "arm_STLEXH", "arm_STLEX",
            "arm_SWP", "arm_SWPB",
            // Elevated load/store multiple instructions.
            "arm_LDM_eret", "arm_LDM_usr",
            "arm_STM_usr",
            // Hint instructions
            "arm_NOP", "arm_PLD_imm", "arm_PLD_reg", "arm_SEV",
            "arm_WFE", "arm_WFI", "arm_YIELD",
            // E, T, J
            "arm_BLX_reg", "arm_BLX_imm", "arm_BXJ", "arm_SETEND",
            // Coprocessor
            "arm_CDP", "arm_LDC", "arm_MCR", "arm_MCRR", "arm_MRC", "arm_MRRC", "arm_STC",
            // System
            "arm_CPS", "arm_RFE", "arm_SRS",
            // Undefined
            "arm_UDF",
            // FPSCR is inaccurate
            "vfp_VMRS",
            // Incorrect Unicorn implementations
            "asimd_VRECPS",         // Unicorn does not fuse the multiply and subtraction, resulting in being off by 1ULP.
            "asimd_VRSQRTS",        // Unicorn does not fuse the multiply and subtraction, resulting in being off by 1ULP.
            "vfp_VCVT_from_fixed",  // Unicorn does not do round-to-nearest-even for this instruction correctly.
        };

        for (const auto& [fn, bitstring] : list) {
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring});
        }
        return InstructionGeneratorInfo{generators, invalid};
    }();

    while (true) {
        const size_t index = RandInt<size_t>(0, instructions.generators.size() - 1);
        const u32 inst = instructions.generators[index].Generate();

        if ((instructions.generators[index].Mask() & 0xF0000000) == 0 && (inst & 0xF0000000) == 0xF0000000) {
            continue;
        }

        if (ShouldTestInst(inst, pc, false, is_last_inst)) {
            return inst;
        }
    }
}

std::vector<u16> GenRandomThumbInst(u32 pc, bool is_last_inst, A32::ITState it_state = {}) {
    static const struct InstructionGeneratorInfo {
        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;
    } instructions = [] {
        const std::vector<std::tuple<std::string, const char*>> list{
#define INST(fn, name, bitstring) {#fn, bitstring},
#include "dynarmic/frontend/A32/decoder/thumb16.inc"
#include "dynarmic/frontend/A32/decoder/thumb32.inc"
#undef INST
        };

        const std::vector<std::tuple<std::string, const char*>> vfp_list{
#define INST(fn, name, bitstring) {#fn, bitstring},
#include "dynarmic/frontend/A32/decoder/vfp.inc"
#undef INST
        };

        const std::vector<std::tuple<std::string, const char*>> asimd_list{
#define INST(fn, name, bitstring) {#fn, bitstring},
#include "dynarmic/frontend/A32/decoder/asimd.inc"
#undef INST
        };

        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;

        // List of instructions not to test
        static constexpr std::array do_not_test{
            "thumb16_BKPT",
            "thumb16_IT",
            "thumb16_SETEND",

            // Exclusive load/stores
            "thumb32_LDREX",
            "thumb32_LDREXB",
            "thumb32_LDREXD",
            "thumb32_LDREXH",
            "thumb32_STREX",
            "thumb32_STREXB",
            "thumb32_STREXD",
            "thumb32_STREXH",

            // FPSCR is inaccurate
            "vfp_VMRS",

            // Unicorn is incorrect?
            "thumb32_MRS_reg",
            "thumb32_MSR_reg",

            // Unicorn has incorrect implementation (incorrect rounding and unsets CPSR.T??)
            "vfp_VCVT_to_fixed",
            "vfp_VCVT_from_fixed",
            "asimd_VRECPS",   // Unicorn does not fuse the multiply and subtraction, resulting in being off by 1ULP.
            "asimd_VRSQRTS",  // Unicorn does not fuse the multiply and subtraction, resulting in being off by 1ULP.

            // Coprocessor
            "thumb32_CDP",
            "thumb32_LDC",
            "thumb32_MCR",
            "thumb32_MCRR",
            "thumb32_MRC",
            "thumb32_MRRC",
            "thumb32_STC",
        };

        for (const auto& [fn, bitstring] : list) {
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring});
        }
        for (const auto& [fn, bs] : vfp_list) {
            std::string bitstring = bs;
            if (bitstring.substr(0, 4) == "cccc" || bitstring.substr(0, 4) == "----") {
                bitstring.replace(0, 4, "1110");
            }
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring.c_str()});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring.c_str()});
        }
        for (const auto& [fn, bs] : asimd_list) {
            std::string bitstring = bs;
            if (bitstring.substr(0, 7) == "1111001") {
                const char U = bitstring[7];
                bitstring.replace(0, 8, "111-1111");
                bitstring[3] = U;
            } else if (bitstring.substr(0, 8) == "11110100") {
                bitstring.replace(0, 8, "11111001");
            } else {
                ASSERT_FALSE("Unhandled ASIMD instruction: {} {}", fn, bs);
            }
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring.c_str()});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring.c_str()});
        }
        return InstructionGeneratorInfo{generators, invalid};
    }();

    while (true) {
        const size_t index = RandInt<size_t>(0, instructions.generators.size() - 1);
        const u32 inst = instructions.generators[index].Generate();
        const bool is_four_bytes = (inst >> 16) != 0;

        if (ShouldTestInst(is_four_bytes ? Common::SwapHalves32(inst) : inst, pc, true, is_last_inst, it_state)) {
            if (is_four_bytes)
                return {static_cast<u16>(inst >> 16), static_cast<u16>(inst)};
            return {static_cast<u16>(inst)};
        }
    }
}

template<typename TestEnv>
Dynarmic::A32::UserConfig GetUserConfig(TestEnv& testenv) {
    Dynarmic::A32::UserConfig user_config;
    user_config.optimizations &= ~OptimizationFlag::FastDispatch;
    user_config.callbacks = &testenv;
    user_config.always_little_endian = true;
    return user_config;
}

template<typename TestEnv>
static void RunTestInstance(Dynarmic::A32::Jit& jit,
                            A32Unicorn<TestEnv>& uni,
                            TestEnv& jit_env,
                            TestEnv& uni_env,
                            const typename A32Unicorn<TestEnv>::RegisterArray& regs,
                            const typename A32Unicorn<TestEnv>::ExtRegArray& vecs,
                            const std::vector<typename TestEnv::InstructionType>& instructions,
                            const u32 cpsr,
                            const u32 fpscr,
                            const size_t ticks_left) {
    const u32 initial_pc = regs[15];
    const u32 num_words = initial_pc / sizeof(typename TestEnv::InstructionType);
    const u32 code_mem_size = num_words + static_cast<u32>(instructions.size());
    const u32 expected_end_pc = code_mem_size * sizeof(typename TestEnv::InstructionType);

    jit_env.code_mem.resize(code_mem_size);
    uni_env.code_mem.resize(code_mem_size);
    std::fill(jit_env.code_mem.begin(), jit_env.code_mem.end(), TestEnv::infinite_loop);
    std::fill(uni_env.code_mem.begin(), uni_env.code_mem.end(), TestEnv::infinite_loop);

    std::copy(instructions.begin(), instructions.end(), jit_env.code_mem.begin() + num_words);
    std::copy(instructions.begin(), instructions.end(), uni_env.code_mem.begin() + num_words);
    jit_env.PadCodeMem();
    uni_env.PadCodeMem();
    jit_env.modified_memory.clear();
    uni_env.modified_memory.clear();
    jit_env.interrupts.clear();
    uni_env.interrupts.clear();

    jit.Regs() = regs;
    jit.ExtRegs() = vecs;
    jit.SetFpscr(fpscr);
    jit.SetCpsr(cpsr);
    jit.ClearCache();
    uni.SetRegisters(regs);
    uni.SetExtRegs(vecs);
    uni.SetFpscr(fpscr);
    uni.EnableFloatingPointAccess();
    uni.SetCpsr(cpsr);
    uni.ClearPageCache();

    jit_env.ticks_left = ticks_left;
    jit.Run();

    uni_env.ticks_left = instructions.size();  // Unicorn counts thumb instructions weirdly.
    uni.Run();

    SCOPE_FAIL {
        fmt::print("Instruction Listing:\n");
        fmt::print("{}\n", Common::DisassembleAArch32(std::is_same_v<TestEnv, ThumbTestEnv>, initial_pc, (const u8*)instructions.data(), instructions.size() * sizeof(instructions[0])));

        fmt::print("Initial register listing:\n");
        for (size_t i = 0; i < regs.size(); ++i) {
            fmt::print("{:3s}: {:08x}\n", static_cast<A32::Reg>(i), regs[i]);
        }
        for (size_t i = 0; i < vecs.size(); ++i) {
            fmt::print("{:3s}: {:08x}\n", static_cast<A32::ExtReg>(i), vecs[i]);
        }
        fmt::print("cpsr {:08x}\n", cpsr);
        fmt::print("fpcr {:08x}\n", fpscr);
        fmt::print("fpcr.AHP   {}\n", FP::FPCR{fpscr}.AHP());
        fmt::print("fpcr.DN    {}\n", FP::FPCR{fpscr}.DN());
        fmt::print("fpcr.FZ    {}\n", FP::FPCR{fpscr}.FZ());
        fmt::print("fpcr.RMode {}\n", static_cast<size_t>(FP::FPCR{fpscr}.RMode()));
        fmt::print("fpcr.FZ16  {}\n", FP::FPCR{fpscr}.FZ16());
        fmt::print("\n");

        fmt::print("Final register listing:\n");
        fmt::print("     unicorn  dynarmic\n");
        const auto uni_regs = uni.GetRegisters();
        for (size_t i = 0; i < regs.size(); ++i) {
            fmt::print("{:3s}: {:08x} {:08x} {}\n", static_cast<A32::Reg>(i), uni_regs[i], jit.Regs()[i], uni_regs[i] != jit.Regs()[i] ? "*" : "");
        }
        const auto uni_ext_regs = uni.GetExtRegs();
        for (size_t i = 0; i < vecs.size(); ++i) {
            fmt::print("s{:2d}: {:08x} {:08x} {}\n", static_cast<size_t>(i), uni_ext_regs[i], jit.ExtRegs()[i], uni_ext_regs[i] != jit.ExtRegs()[i] ? "*" : "");
        }
        fmt::print("cpsr {:08x} {:08x} {}\n", uni.GetCpsr(), jit.Cpsr(), uni.GetCpsr() != jit.Cpsr() ? "*" : "");
        fmt::print("fpsr {:08x} {:08x} {}\n", uni.GetFpscr(), jit.Fpscr(), (uni.GetFpscr() & 0xF0000000) != (jit.Fpscr() & 0xF0000000) ? "*" : "");
        fmt::print("\n");

        fmt::print("Modified memory:\n");
        fmt::print("                 uni dyn\n");
        auto uni_iter = uni_env.modified_memory.begin();
        auto jit_iter = jit_env.modified_memory.begin();
        while (uni_iter != uni_env.modified_memory.end() || jit_iter != jit_env.modified_memory.end()) {
            if (uni_iter == uni_env.modified_memory.end() || (jit_iter != jit_env.modified_memory.end() && uni_iter->first > jit_iter->first)) {
                fmt::print("{:08x}:    {:02x} *\n", jit_iter->first, jit_iter->second);
                jit_iter++;
            } else if (jit_iter == jit_env.modified_memory.end() || jit_iter->first > uni_iter->first) {
                fmt::print("{:08x}: {:02x}    *\n", uni_iter->first, uni_iter->second);
                uni_iter++;
            } else if (uni_iter->first == jit_iter->first) {
                fmt::print("{:08x}: {:02x} {:02x} {}\n", uni_iter->first, uni_iter->second, jit_iter->second, uni_iter->second != jit_iter->second ? "*" : "");
                uni_iter++;
                jit_iter++;
            }
        }
        fmt::print("\n");

        fmt::print("x86_64:\n");
        jit.DumpDisassembly();

        fmt::print("Interrupts:\n");
        for (const auto& i : uni_env.interrupts) {
            std::puts(i.c_str());
        }
    };

    REQUIRE(uni_env.code_mem_modified_by_guest == jit_env.code_mem_modified_by_guest);
    if (uni_env.code_mem_modified_by_guest) {
        return;
    }

    // Qemu doesn't do Thumb transitions??
    {
        const u32 uni_pc = uni.GetPC();
        const bool is_thumb = (jit.Cpsr() & (1 << 5)) != 0;
        const u32 new_uni_pc = uni_pc & (is_thumb ? 0xFFFFFFFE : 0xFFFFFFFC);
        uni.SetPC(new_uni_pc);
    }

    if (uni.GetRegisters()[15] > jit.Regs()[15]) {
        const u32 final_pc = jit.Regs()[15];
        if (final_pc >= initial_pc && final_pc < expected_end_pc) {
            fmt::print("Warning: Possible unicorn overrrun, attempt recovery\n");
            jit.Step();
        }
    }

    REQUIRE(uni.GetRegisters() == jit.Regs());
    REQUIRE(uni.GetExtRegs() == jit.ExtRegs());
    REQUIRE((uni.GetCpsr() & 0xFFFFFDDF) == (jit.Cpsr() & 0xFFFFFDDF));
    REQUIRE((uni.GetFpscr() & 0xF8000000) == (jit.Fpscr() & 0xF8000000));
    REQUIRE(uni_env.modified_memory == jit_env.modified_memory);
    REQUIRE(uni_env.interrupts.empty());
}
}  // Anonymous namespace

TEST_CASE("A32: Single random arm instruction", "[arm]") {
    ArmTestEnv jit_env{};
    ArmTestEnv uni_env{};

    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};
    A32Unicorn<ArmTestEnv> uni{uni_env};

    A32Unicorn<ArmTestEnv>::RegisterArray regs;
    A32Unicorn<ArmTestEnv>::ExtRegArray ext_reg;
    std::vector<u32> instructions(1);

    for (size_t iteration = 0; iteration < 100000; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        instructions[0] = GenRandomArmInst(0, true);

        const u32 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28) | 0x10;
        const u32 fpcr = RandomFpcr();

        INFO("Instruction: 0x" << std::hex << instructions[0]);

        regs[15] = start_address;
        RunTestInstance(jit, uni, jit_env, uni_env, regs, ext_reg, instructions, cpsr, fpcr, 1);
    }
}

TEST_CASE("A32: Small random arm block", "[arm]") {
    ArmTestEnv jit_env{};
    ArmTestEnv uni_env{};

    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};
    A32Unicorn<ArmTestEnv> uni{uni_env};

    A32Unicorn<ArmTestEnv>::RegisterArray regs;
    A32Unicorn<ArmTestEnv>::ExtRegArray ext_reg;
    std::vector<u32> instructions(5);

    for (size_t iteration = 0; iteration < 100000; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        instructions[0] = GenRandomArmInst(0, false);
        instructions[1] = GenRandomArmInst(4, false);
        instructions[2] = GenRandomArmInst(8, false);
        instructions[3] = GenRandomArmInst(12, false);
        instructions[4] = GenRandomArmInst(16, true);

        const u32 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28) | 0x10;
        const u32 fpcr = RandomFpcr();

        INFO("Instruction 1: 0x" << std::hex << instructions[0]);
        INFO("Instruction 2: 0x" << std::hex << instructions[1]);
        INFO("Instruction 3: 0x" << std::hex << instructions[2]);
        INFO("Instruction 4: 0x" << std::hex << instructions[3]);
        INFO("Instruction 5: 0x" << std::hex << instructions[4]);

        regs[15] = start_address;
        RunTestInstance(jit, uni, jit_env, uni_env, regs, ext_reg, instructions, cpsr, fpcr, 5);
    }
}

TEST_CASE("A32: Large random arm block", "[arm]") {
    ArmTestEnv jit_env{};
    ArmTestEnv uni_env{};

    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};
    A32Unicorn<ArmTestEnv> uni{uni_env};

    A32Unicorn<ArmTestEnv>::RegisterArray regs;
    A32Unicorn<ArmTestEnv>::ExtRegArray ext_reg;

    constexpr size_t instruction_count = 100;
    std::vector<u32> instructions(instruction_count);

    for (size_t iteration = 0; iteration < 10000; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        for (size_t j = 0; j < instruction_count; ++j) {
            instructions[j] = GenRandomArmInst(j * 4, j == instruction_count - 1);
        }

        const u64 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28) | 0x10;
        const u32 fpcr = RandomFpcr();

        regs[15] = start_address;
        RunTestInstance(jit, uni, jit_env, uni_env, regs, ext_reg, instructions, cpsr, fpcr, 100);
    }
}

TEST_CASE("A32: Single random thumb instruction", "[thumb]") {
    ThumbTestEnv jit_env{};
    ThumbTestEnv uni_env{};

    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};
    A32Unicorn<ThumbTestEnv> uni{uni_env};

    A32Unicorn<ThumbTestEnv>::RegisterArray regs;
    A32Unicorn<ThumbTestEnv>::ExtRegArray ext_reg;
    std::vector<u16> instructions;

    for (size_t iteration = 0; iteration < 100000; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        instructions = GenRandomThumbInst(0, true);

        const u32 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28) | 0x1F0;
        const u32 fpcr = RandomFpcr();

        INFO("Instruction: 0x" << std::hex << instructions[0]);

        regs[15] = start_address;
        RunTestInstance(jit, uni, jit_env, uni_env, regs, ext_reg, instructions, cpsr, fpcr, 1);
    }
}

TEST_CASE("A32: Small random thumb block", "[thumb]") {
    ThumbTestEnv jit_env{};
    ThumbTestEnv uni_env{};

    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};
    A32Unicorn<ThumbTestEnv> uni{uni_env};

    A32Unicorn<ThumbTestEnv>::RegisterArray regs;
    A32Unicorn<ThumbTestEnv>::ExtRegArray ext_reg;
    std::vector<u16> instructions;

    for (size_t iteration = 0; iteration < 100000; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        instructions.clear();
        for (size_t i = 0; i < 5; i++) {
            const std::vector<u16> inst = GenRandomThumbInst(instructions.size() * 2, i == 4);
            instructions.insert(instructions.end(), inst.begin(), inst.end());
        }

        const u32 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28) | 0x1F0;
        const u32 fpcr = RandomFpcr();

        regs[15] = start_address;
        RunTestInstance(jit, uni, jit_env, uni_env, regs, ext_reg, instructions, cpsr, fpcr, 5);
    }
}

TEST_CASE("A32: Test thumb IT instruction", "[thumb]") {
    ThumbTestEnv jit_env{};
    ThumbTestEnv uni_env{};

    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};
    A32Unicorn<ThumbTestEnv> uni{uni_env};

    A32Unicorn<ThumbTestEnv>::RegisterArray regs;
    A32Unicorn<ThumbTestEnv>::ExtRegArray ext_reg;
    std::vector<u16> instructions;

    for (size_t iteration = 0; iteration < 100000; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        const size_t pre_instructions = RandInt<size_t>(0, 3);
        const size_t post_instructions = RandInt<size_t>(5, 8);

        instructions.clear();

        for (size_t i = 0; i < pre_instructions; i++) {
            const std::vector<u16> inst = GenRandomThumbInst(instructions.size() * 2, false);
            instructions.insert(instructions.end(), inst.begin(), inst.end());
        }

        // Emit IT instruction
        A32::ITState it_state = [&] {
            while (true) {
                const u16 imm8 = RandInt<u16>(0, 0xFF);
                if (Common::Bits<0, 3>(imm8) == 0b0000 || Common::Bits<4, 7>(imm8) == 0b1111 || (Common::Bits<4, 7>(imm8) == 0b1110 && Common::BitCount(Common::Bits<0, 3>(imm8)) != 1)) {
                    continue;
                }
                instructions.push_back(0b1011111100000000 | imm8);
                return A32::ITState{static_cast<u8>(imm8)};
            }
        }();

        for (size_t i = 0; i < post_instructions; i++) {
            const std::vector<u16> inst = GenRandomThumbInst(instructions.size() * 2, i == post_instructions - 1, it_state);
            instructions.insert(instructions.end(), inst.begin(), inst.end());
            it_state = it_state.Advance();
        }

        const u32 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28) | 0x1F0;
        const u32 fpcr = RandomFpcr();

        regs[15] = start_address;
        RunTestInstance(jit, uni, jit_env, uni_env, regs, ext_reg, instructions, cpsr, fpcr, pre_instructions + 1 + post_instructions);
    }
}
