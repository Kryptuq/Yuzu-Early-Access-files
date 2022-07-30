/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <catch2/catch.hpp>
#include <mcl/stdint.hpp>

#include "./testenv.h"
#include "dynarmic/interface/A32/a32.h"

static Dynarmic::A32::UserConfig GetUserConfig(ThumbTestEnv* testenv) {
    Dynarmic::A32::UserConfig user_config;
    user_config.callbacks = testenv;
    return user_config;
}

TEST_CASE("thumb: lsls r0, r1, #2", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x0088,  // lsls r0, r1, #2
        0xE7FE,  // b +#0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x00000030);  // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 8);
    REQUIRE(jit.Regs()[1] == 2);
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb: lsls r0, r1, #31", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x07C8,  // lsls r0, r1, #31
        0xE7FE,  // b +#0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0xFFFFFFFF;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x00000030);  // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x80000000);
    REQUIRE(jit.Regs()[1] == 0xffffffff);
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0xA0000030);  // N, C flags, Thumb, User-mode
}

TEST_CASE("thumb: revsh r4, r3", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xBADC,  // revsh r4, r3
        0xE7FE,  // b +#0
    };

    jit.Regs()[3] = 0x12345678;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x00000030);  // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[3] == 0x12345678);
    REQUIRE(jit.Regs()[4] == 0x00007856);
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0x00000030);  // Thumb, User-mode
}

TEST_CASE("thumb: ldr r3, [r3, #28]", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x69DB,  // ldr r3, [r3, #28]
        0xE7FE,  // b +#0
    };

    jit.Regs()[3] = 0x12345678;
    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x00000030);  // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[3] == 0x97969594);  // Memory location 0x12345694
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0x00000030);  // Thumb, User-mode
}

TEST_CASE("thumb: blx +#67712", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xF010, 0xEC3E,  // blx +#67712
        0xE7FE           // b +#0
    };

    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x00000030);  // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[14] == (0x4 | 1));
    REQUIRE(jit.Regs()[15] == 0x10880);
    REQUIRE(jit.Cpsr() == 0x00000010);  // User-mode
}

TEST_CASE("thumb: bl +#234584", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xF039, 0xFA2A,  // bl +#234584
        0xE7FE           // b +#0
    };

    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x00000030);  // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[14] == (0x4 | 1));
    REQUIRE(jit.Regs()[15] == 0x39458);
    REQUIRE(jit.Cpsr() == 0x00000030);  // Thumb, User-mode
}

TEST_CASE("thumb: bl -#42", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xF7FF, 0xFFE9,  // bl -#42
        0xE7FE           // b +#0
    };

    jit.Regs()[15] = 0;       // PC = 0
    jit.SetCpsr(0x00000030);  // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[14] == (0x4 | 1));
    REQUIRE(jit.Regs()[15] == 0xFFFFFFD6);
    REQUIRE(jit.Cpsr() == 0x00000030);  // Thumb, User-mode
}

TEST_CASE("thumb: Opt Failure: Get/Set Elimination for Flags", "[thumb]") {
    // This was a randomized test-case that was failing.
    //
    // Incorrect IR:
    // Block: location={0000000100000000}
    // cycles=6, entry_cond=al
    // [0000556569455160] %0     = GetRegister r1 (uses: 1)
    // [00005565694551c8] %1     = GetRegister r6 (uses: 1)
    // [0000556569455230] %2     = Mul32 %1, %0 (uses: 1)
    // [0000556569455298]          SetRegister r6, %2 (uses: 0)
    // [0000556569455300]          Void (uses: 0)
    // [00005565694553d0]          Void (uses: 0)
    // [0000556569455438]          Void (uses: 0)
    // [00005565694554a0]          Void (uses: 0)
    // [0000556569455508]          Void (uses: 0)
    // [00005565694555d8] %9     = GetCFlag (uses: 1)
    // [0000556569455640] %10    = GetRegister r3 (uses: 2)
    // [00005565694556a8] %11    = Identity %10 (uses: 1)
    // [0000556569455710] %12    = Add32 %11, %10, %9 (uses: 2)
    // [0000556569455778]          SetRegister r3, %12 (uses: 0)
    // [00005565694557e0] %14    = GetNZCVFromOp %12 (uses: 1)
    // [0000556569455848]          SetCpsrNZCV %14 (uses: 0)
    // [00005565694558b0] %16    = GetRegister sp (uses: 1)
    // [0000556569455918] %17    = Add32 %16, #0x2c4, #0 (uses: 1)
    // [0000556569455980] %18    = GetRegister r4 (uses: 1)
    // [00005565694559e8]          WriteMemory32 #0x100000006, %17, %18, <unknown immediate type> (uses: 0)
    // [0000556569455a50] %20    = GetRegister r2 (uses: 1)
    // [0000556569455ab8] %21    = GetRegister r5 (uses: 1)
    // [0000556569455b20] %22    = Add32 %21, %20, #0 (uses: 1)
    // [0000556569455b88]          SetRegister r5, %22 (uses: 0)
    // terminal = LinkBlock{{000000010000000a}}

    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x434e,  // muls r6, r1, r6
        0x4557,  // cmp r7, r10
        0x415b,  // adcs r3, r3
        0x94b1,  // str r4, [sp, #708]
        0x4415,  // add r5, r2
        0xe7fe   // b +#0
    };

    jit.Regs() = {0x2154abb5, 0xdbaa6333, 0xf8a7bc0e, 0x989f6096, 0x19cd7783, 0xe1cf5b7f, 0x9bb1aa6c, 0x6b700f5c,
                  0xc04f6cb2, 0xc8df07f0, 0x217d83de, 0xe77fdffa, 0x98bcceaf, 0xbfcab4f7, 0xdb9d5405, 0x00000000};
    jit.SetCpsr(0x000001f0);  // Thumb, User-mode

    test_env.ticks_left = 7;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x2154abb5);
    REQUIRE(jit.Regs()[1] == 0xdbaa6333);
    REQUIRE(jit.Regs()[2] == 0xf8a7bc0e);
    REQUIRE(jit.Regs()[3] == 0x313ec12d);
    REQUIRE(jit.Regs()[4] == 0x19cd7783);
    REQUIRE(jit.Regs()[5] == 0xda77178d);
    REQUIRE(jit.Regs()[6] == 0x4904b784);
    REQUIRE(jit.Regs()[7] == 0x6b700f5c);
    REQUIRE(jit.Regs()[8] == 0xc04f6cb2);
    REQUIRE(jit.Regs()[9] == 0xc8df07f0);
    REQUIRE(jit.Regs()[10] == 0x217d83de);
    REQUIRE(jit.Regs()[11] == 0xe77fdffa);
    REQUIRE(jit.Regs()[12] == 0x98bcceaf);
    REQUIRE(jit.Regs()[13] == 0xbfcab4f7);
    REQUIRE(jit.Regs()[14] == 0xdb9d5405);
    REQUIRE(jit.Regs()[15] == 0x0000000a);
    REQUIRE(jit.Cpsr() == 0x300001f0);
}

TEST_CASE("thumb: Opt Failure: Get/Set Elimination for Flags 2", "[thumb]") {
    // This was a randomized test-case that was failing.

    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x442a,  // add r2, r5
        0x065d,  // lsls r5, r3, #25
        0xbc64,  // pop {r2, r5, r6}
        0x2666,  // movs r6, #102
        0x7471,  // strb r1, [r6, #17]
        0xe7fe   // b +#0
    };

    jit.Regs() = {0x954d53b0, 0x4caaad40, 0xa42325b8, 0x0da0cdb6, 0x0f43507e, 0x31d68ae1, 0x9c471808, 0x892a6888,
                  0x3b9ffb23, 0x0a92ef93, 0x38dee619, 0xc0e95e81, 0x6a448690, 0xc2d4d6ad, 0xe93600b9, 0x00000000};
    jit.SetCpsr(0x000001f0);  // Thumb, User-mode

    test_env.ticks_left = 7;
    jit.Run();

    const std::array<u32, 16> expected = {0x954d53b0, 0x4caaad40, 0xb0afaead, 0x0da0cdb6, 0x0f43507e, 0xb4b3b2b1, 0x00000066, 0x892a6888,
                                          0x3b9ffb23, 0x0a92ef93, 0x38dee619, 0xc0e95e81, 0x6a448690, 0xc2d4d6b9, 0xe93600b9, 0x0000000a};
    REQUIRE(jit.Regs() == expected);
    REQUIRE(jit.Cpsr() == 0x200001f0);
}
