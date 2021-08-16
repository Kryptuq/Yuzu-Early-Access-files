/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <catch2/catch.hpp>

#include "./testenv.h"
#include "dynarmic/frontend/A32/location_descriptor.h"
#include "dynarmic/interface/A32/a32.h"

using namespace Dynarmic;

static A32::UserConfig GetUserConfig(ArmTestEnv* testenv) {
    A32::UserConfig user_config;
    user_config.optimizations &= ~OptimizationFlag::FastDispatch;
    user_config.callbacks = testenv;
    return user_config;
}

TEST_CASE("arm: Opt Failure: Const folding in MostSignificantWord", "[arm][A32]") {
    // This was a randomized test-case that was failing.
    // This was due to constant folding for MostSignificantWord
    // failing to take into account an associated GetCarryFromOp
    // pseudoinstruction.

    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe30ad071,  // movw, sp, #41073
        0xe75efd3d,  // smmulr lr, sp, sp
        0xa637af1e,  // shadd16ge r10, r7, lr
        0xf57ff01f,  // clrex
        0x86b98879,  // sxtahhi r8, r9, r9, ror #16
        0xeafffffe,  // b +#0
    };

    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 6;
    jit.Run();

    // If we don't trigger the GetCarryFromOp ASSERT, we're fine.
}

TEST_CASE("arm: Unintended modification in SetCFlag", "[arm][A32]") {
    // This was a randomized test-case that was failing.
    //
    // IR produced for location {12, !T, !E} was:
    // %0     = GetRegister r1
    // %1     = SubWithCarry %0, #0x3e80000, #1
    // %2     = GetCarryFromOp %1
    // %3     = GetOverflowFromOp %1
    // %4     = MostSignificantBit %1
    //          SetNFlag %4
    // %6     = IsZero %1
    //          SetZFlag %6
    //          SetCFlag %2
    //          SetVFlag %3
    // %10    = GetRegister r5
    // %11    = AddWithCarry %10, #0x8a00, %2
    //          SetRegister r4, %11
    //
    // The reference to %2 in instruction %11 was the issue, because instruction %8
    // told the register allocator it was a Use but then modified the value.
    // Changing the EmitSet*Flag instruction to declare their arguments as UseScratch
    // solved this bug.

    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe35f0cd9,  // cmp pc, #55552
        0xe11c0474,  // tst r12, r4, ror r4
        0xe1a006a7,  // mov r0, r7, lsr #13
        0xe35107fa,  // cmp r1, #0x3E80000
        0xe2a54c8a,  // adc r4, r5, #35328
        0xeafffffe,  // b +#0
    };

    jit.Regs() = {
        0x6973b6bb, 0x267ea626, 0x69debf49, 0x8f976895, 0x4ecd2d0d, 0xcf89b8c7, 0xb6713f85, 0x15e2aa5,
        0xcd14336a, 0xafca0f3e, 0xace2efd9, 0x68fb82cd, 0x775447c0, 0xc9e1f8cd, 0xebe0e626, 0x0};
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 6;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x00000af1);
    REQUIRE(jit.Regs()[1] == 0x267ea626);
    REQUIRE(jit.Regs()[2] == 0x69debf49);
    REQUIRE(jit.Regs()[3] == 0x8f976895);
    REQUIRE(jit.Regs()[4] == 0xcf8a42c8);
    REQUIRE(jit.Regs()[5] == 0xcf89b8c7);
    REQUIRE(jit.Regs()[6] == 0xb6713f85);
    REQUIRE(jit.Regs()[7] == 0x015e2aa5);
    REQUIRE(jit.Regs()[8] == 0xcd14336a);
    REQUIRE(jit.Regs()[9] == 0xafca0f3e);
    REQUIRE(jit.Regs()[10] == 0xace2efd9);
    REQUIRE(jit.Regs()[11] == 0x68fb82cd);
    REQUIRE(jit.Regs()[12] == 0x775447c0);
    REQUIRE(jit.Regs()[13] == 0xc9e1f8cd);
    REQUIRE(jit.Regs()[14] == 0xebe0e626);
    REQUIRE(jit.Regs()[15] == 0x00000014);
    REQUIRE(jit.Cpsr() == 0x200001d0);
}

TEST_CASE("arm: shsax (Edge-case)", "[arm][A32]") {
    // This was a randomized test-case that was failing.
    //
    // The issue here was one of the words to be subtracted was 0x8000.
    // When the 2s complement was calculated by (~a + 1), it was 0x8000.

    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe63dbf59,  // shsax r11, sp, r9
        0xeafffffe,  // b +#0
    };

    jit.Regs() = {
        0x3a3b8b18, 0x96156555, 0xffef039f, 0xafb946f2, 0x2030a69a, 0xafe09b2a, 0x896823c8, 0xabde0ded,
        0x9825d6a6, 0x17498000, 0x999d2c95, 0x8b812a59, 0x209bdb58, 0x2f7fb1d4, 0x0f378107, 0x00000000};
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x3a3b8b18);
    REQUIRE(jit.Regs()[1] == 0x96156555);
    REQUIRE(jit.Regs()[2] == 0xffef039f);
    REQUIRE(jit.Regs()[3] == 0xafb946f2);
    REQUIRE(jit.Regs()[4] == 0x2030a69a);
    REQUIRE(jit.Regs()[5] == 0xafe09b2a);
    REQUIRE(jit.Regs()[6] == 0x896823c8);
    REQUIRE(jit.Regs()[7] == 0xabde0ded);
    REQUIRE(jit.Regs()[8] == 0x9825d6a6);
    REQUIRE(jit.Regs()[9] == 0x17498000);
    REQUIRE(jit.Regs()[10] == 0x999d2c95);
    REQUIRE(jit.Regs()[11] == 0x57bfe48e);
    REQUIRE(jit.Regs()[12] == 0x209bdb58);
    REQUIRE(jit.Regs()[13] == 0x2f7fb1d4);
    REQUIRE(jit.Regs()[14] == 0x0f378107);
    REQUIRE(jit.Regs()[15] == 0x00000004);
    REQUIRE(jit.Cpsr() == 0x000001d0);
}

TEST_CASE("arm: uasx (Edge-case)", "[arm][A32]") {
    // UASX's Rm<31:16> == 0x0000.
    // An implementation that depends on addition overflow to detect
    // if diff >= 0 will fail this testcase.

    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe6549f35,  // uasx r9, r4, r5
        0xeafffffe,  // b +#0
    };

    jit.Regs()[4] = 0x8ed38f4c;
    jit.Regs()[5] = 0x0000261d;
    jit.Regs()[15] = 0x00000000;
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[4] == 0x8ed38f4c);
    REQUIRE(jit.Regs()[5] == 0x0000261d);
    REQUIRE(jit.Regs()[9] == 0xb4f08f4c);
    REQUIRE(jit.Regs()[15] == 0x00000004);
    REQUIRE(jit.Cpsr() == 0x000301d0);
}

TEST_CASE("arm: smuad (Edge-case)", "[arm][A32]") {
    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xE700F211,  // smuad r0, r1, r2
        0xeafffffe,  // b +#0
    };

    jit.Regs() = {
        0,           // Rd
        0x80008000,  // Rn
        0x80008000,  // Rm
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    };
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x80000000);
    REQUIRE(jit.Regs()[1] == 0x80008000);
    REQUIRE(jit.Regs()[2] == 0x80008000);
    REQUIRE(jit.Cpsr() == 0x080001d0);
}

TEST_CASE("arm: Test InvalidateCacheRange", "[arm][A32]") {
    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe3a00005,  // mov r0, #5
        0xe3a0100D,  // mov r1, #13
        0xe0812000,  // add r2, r1, r0
        0xeafffffe,  // b +#0 (infinite loop)
    };

    jit.Regs() = {};
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 5);
    REQUIRE(jit.Regs()[1] == 13);
    REQUIRE(jit.Regs()[2] == 18);
    REQUIRE(jit.Regs()[15] == 0x0000000c);
    REQUIRE(jit.Cpsr() == 0x000001d0);

    // Change the code
    test_env.code_mem[1] = 0xe3a01007;  // mov r1, #7
    jit.InvalidateCacheRange(/*start_memory_location = */ 4, /* length_in_bytes = */ 4);

    // Reset position of PC
    jit.Regs()[15] = 0;

    test_env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 5);
    REQUIRE(jit.Regs()[1] == 7);
    REQUIRE(jit.Regs()[2] == 12);
    REQUIRE(jit.Regs()[15] == 0x0000000c);
    REQUIRE(jit.Cpsr() == 0x000001d0);
}

TEST_CASE("arm: Step blx", "[arm]") {
    ArmTestEnv test_env;
    A32::UserConfig config = GetUserConfig(&test_env);
    config.optimizations |= OptimizationFlag::FastDispatch;
    Dynarmic::A32::Jit jit{config};
    test_env.code_mem = {
        0xe12fff30,  // blx r0
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xeafffffe,  // b +#0 (infinite loop)
    };

    jit.Regs()[0] = 8;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 10;
    jit.Step();

    REQUIRE(jit.Regs()[0] == 8);
    REQUIRE(jit.Regs()[14] == 4);
    REQUIRE(jit.Regs()[15] == 8);
    REQUIRE(jit.Cpsr() == 0x000001d0);
}

TEST_CASE("arm: Step bx", "[arm]") {
    ArmTestEnv test_env;
    A32::UserConfig config = GetUserConfig(&test_env);
    config.optimizations |= OptimizationFlag::FastDispatch;
    Dynarmic::A32::Jit jit{config};
    test_env.code_mem = {
        0xe12fff10,  // bx r0
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xeafffffe,  // b +#0 (infinite loop)
    };

    jit.Regs()[0] = 8;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 10;
    jit.Step();

    REQUIRE(jit.Regs()[0] == 8);
    REQUIRE(jit.Regs()[15] == 8);
    REQUIRE(jit.Cpsr() == 0x000001d0);
}

TEST_CASE("arm: Test stepping", "[arm]") {
    ArmTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xeafffffe,  // b +#0 (infinite loop)
    };

    jit.Regs()[0] = 8;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x000001d0);  // User-mode

    for (size_t i = 0; i < 5; ++i) {
        test_env.ticks_left = 10;
        jit.Step();

        REQUIRE(jit.Regs()[15] == (i + 1) * 4);
        REQUIRE(jit.Cpsr() == 0x000001d0);
    }

    test_env.ticks_left = 20;
    jit.Run();

    REQUIRE(jit.Regs()[15] == 80);
    REQUIRE(jit.Cpsr() == 0x000001d0);
}

TEST_CASE("arm: Test stepping 2", "[arm]") {
    ArmTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe12fff10,  // bx r0
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xeafffffe,  // b +#0 (infinite loop)
    };

    jit.Regs()[0] = 4;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x000001d0);  // User-mode

    for (size_t i = 0; i < 5; ++i) {
        test_env.ticks_left = 10;
        jit.Step();

        REQUIRE(jit.Regs()[15] == (i + 1) * 4);
        REQUIRE(jit.Cpsr() == 0x000001d0);
    }

    test_env.ticks_left = 20;
    jit.Run();

    REQUIRE(jit.Regs()[15] == 80);
    REQUIRE(jit.Cpsr() == 0x000001d0);
}

TEST_CASE("arm: Test stepping 3", "[arm]") {
    ArmTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe12fff10,  // bx r0
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop
        0xe320f000,  // nop

        0xeafffffe,  // b +#0 (infinite loop)
    };

    jit.Regs()[0] = 4;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 10;
    jit.Step();

    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x000001d0);

    test_env.ticks_left = 20;
    jit.Run();

    REQUIRE(jit.Regs()[15] == 20);
    REQUIRE(jit.Cpsr() == 0x000001d0);
}

TEST_CASE("arm: PackedAbsDiffSumS8", "[arm][A32]") {
    // This was a randomized test-case that was failing.
    // In circumstances there were cases when the upper 32 bits of an argument to psadbw were not zero.

    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x87414354,  // smlsldhi r4, r1, r4, r3
        0xe7886412,  // usad8a r8, r2, r4, r6
        0xeafffffe,  // b +#0
    };

    jit.Regs() = {
        0xea85297c,
        0x417ad918,
        0x64f8b70b,
        0xcca0373e,
        0xbc722361,
        0xc528c69e,
        0xca926de8,
        0xd665d210,
        0xb5650555,
        0x4a24b25b,
        0xaed44144,
        0xe87230b2,
        0x98e391de,
        0x126efc0c,
        0xe591fd11,
        0x00000000,
    };
    jit.SetCpsr(0xb0000010);

    test_env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0xea85297c);
    REQUIRE(jit.Regs()[1] == 0x417ad918);
    REQUIRE(jit.Regs()[2] == 0x64f8b70b);
    REQUIRE(jit.Regs()[3] == 0xcca0373e);
    REQUIRE(jit.Regs()[4] == 0xb685ec9f);
    REQUIRE(jit.Regs()[5] == 0xc528c69e);
    REQUIRE(jit.Regs()[6] == 0xca926de8);
    REQUIRE(jit.Regs()[7] == 0xd665d210);
    REQUIRE(jit.Regs()[8] == 0xca926f76);
    REQUIRE(jit.Regs()[9] == 0x4a24b25b);
    REQUIRE(jit.Regs()[10] == 0xaed44144);
    REQUIRE(jit.Regs()[11] == 0xe87230b2);
    REQUIRE(jit.Regs()[12] == 0x98e391de);
    REQUIRE(jit.Regs()[13] == 0x126efc0c);
    REQUIRE(jit.Regs()[14] == 0xe591fd11);
    REQUIRE(jit.Regs()[15] == 0x00000008);
    REQUIRE(jit.Cpsr() == 0xb0000010);
}

TEST_CASE("arm: vclt.f32 with zero", "[arm][A32]") {
    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xf3b93628,  // vclt.f32 d3, d24, #0
        0xeafffffe,  // b +#0
    };

    jit.ExtRegs()[48] = 0x3a87d9f1;
    jit.ExtRegs()[49] = 0x80796dc0;

    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.ExtRegs()[6] == 0x00000000);
    REQUIRE(jit.ExtRegs()[7] == 0x00000000);
}

TEST_CASE("arm: vcvt.s16.f64", "[arm][A32]") {
    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xeebe8b45,  // vcvt.s16.f64 d8, d8, #6
        0xeafffffe,  // b +#0
    };

    jit.ExtRegs()[16] = 0x9a7110b0;
    jit.ExtRegs()[17] = 0xcd78f4e7;

    jit.SetCpsr(0x000001d0);  // User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.ExtRegs()[16] == 0xffff8000);
    REQUIRE(jit.ExtRegs()[17] == 0xffffffff);
}

TEST_CASE("arm: Memory access (fastmem)", "[arm][A32]") {
    constexpr size_t address_width = 12;
    constexpr size_t memory_size = 1ull << address_width;  // 4K
    constexpr size_t page_size = 4 * 1024;
    constexpr size_t buffer_size = 2 * page_size;
    char buffer[buffer_size];

    void* buffer_ptr = reinterpret_cast<void*>(buffer);
    size_t buffer_size_nconst = buffer_size;
    char* backing_memory = reinterpret_cast<char*>(std::align(page_size, memory_size, buffer_ptr, buffer_size_nconst));

    A32FastmemTestEnv env{backing_memory};
    Dynarmic::A32::UserConfig config{&env};
    config.fastmem_pointer = backing_memory;
    config.recompile_on_fastmem_failure = false;
    config.processor_id = 0;

    Dynarmic::A32::Jit jit{config};
    memset(backing_memory, 0, memory_size);
    memcpy(backing_memory + 0x100, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.", 57);

    env.MemoryWrite32(0, 0xE5904000);  // LDR R4, [R0]
    env.MemoryWrite32(4, 0xE5814000);  // STR R4, [R1]
    env.MemoryWrite32(8, 0xEAFFFFFE);  // B .
    jit.Regs()[0] = 0x100;
    jit.Regs()[1] = 0x1F0;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x000001d0);  // User-mode
    env.ticks_left = 3;

    jit.Run();
    REQUIRE(strncmp(backing_memory + 0x100, backing_memory + 0x1F0, 4) == 0);
}
