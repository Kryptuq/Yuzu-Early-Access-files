/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <catch2/catch.hpp>

#include "./testenv.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/interface/exclusive_monitor.h"

using namespace Dynarmic;

TEST_CASE("A64: ADD", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x8b020020);  // ADD X0, X1, X2
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetRegister(0, 0);
    jit.SetRegister(1, 1);
    jit.SetRegister(2, 2);
    jit.SetPC(0);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetRegister(0) == 3);
    REQUIRE(jit.GetRegister(1) == 1);
    REQUIRE(jit.GetRegister(2) == 2);
    REQUIRE(jit.GetPC() == 4);
}

TEST_CASE("A64: ADD{V,P}", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x0E31B801);  // ADDV b1, v0.8b
    env.code_mem.emplace_back(0x4E31B802);  // ADDV b2, v0.16b
    env.code_mem.emplace_back(0x0E71B803);  // ADDV h3, v0.4h
    env.code_mem.emplace_back(0x4E71B804);  // ADDV h4, v0.8h
    env.code_mem.emplace_back(0x0EA0BC05);  // ADDP v5.2s, v0.2s, v0.2s
    env.code_mem.emplace_back(0x4EB1B806);  // ADDV s6, v0.4s
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetVector(0, {0x0101010101010101, 0x0101010101010101});
    jit.SetPC(0);

    env.ticks_left = 7;
    jit.Run();

    REQUIRE(jit.GetVector(1) == Vector{0x0000000000000008, 0x0000000000000000});
    REQUIRE(jit.GetVector(2) == Vector{0x0000000000000010, 0x0000000000000000});
    REQUIRE(jit.GetVector(3) == Vector{0x0000000000000404, 0x0000000000000000});
    REQUIRE(jit.GetVector(4) == Vector{0x0000000000000808, 0x0000000000000000});
    REQUIRE(jit.GetVector(5) == Vector{0x0202020202020202, 0x0000000000000000});
    REQUIRE(jit.GetVector(6) == Vector{0x0000000004040404, 0x0000000000000000});
}

TEST_CASE("A64: UADDL{V,P}", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x2E303801);  // UADDLV h1, v0.8b
    env.code_mem.emplace_back(0x6E303802);  // UADDLV h2, v0.16b
    env.code_mem.emplace_back(0x2E703803);  // UADDLV s3, v0.4h
    env.code_mem.emplace_back(0x6E703804);  // UADDLV s4, v0.8h
    env.code_mem.emplace_back(0x2EA02805);  // UADDLP v5.1d, v0.2s
    env.code_mem.emplace_back(0x6EB03806);  // UADDLV d6, v0.4s
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetVector(0, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF});
    jit.SetPC(0);

    env.ticks_left = 7;
    jit.Run();

    REQUIRE(jit.GetVector(1) == Vector{0x00000000000007f8, 0x0000000000000000});
    REQUIRE(jit.GetVector(2) == Vector{0x0000000000000ff0, 0x0000000000000000});
    REQUIRE(jit.GetVector(3) == Vector{0x000000000003fffc, 0x0000000000000000});
    REQUIRE(jit.GetVector(4) == Vector{0x000000000007fff8, 0x0000000000000000});
    REQUIRE(jit.GetVector(5) == Vector{0x00000001fffffffe, 0x0000000000000000});
    REQUIRE(jit.GetVector(6) == Vector{0x00000003fffffffc, 0x0000000000000000});
}

TEST_CASE("A64: SADDL{V,P}", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x0E303801);  // SADDLV h1, v0.8b
    env.code_mem.emplace_back(0x4E303802);  // SADDLV h2, v0.16b
    env.code_mem.emplace_back(0x0E703803);  // SADDLV s3, v0.4h
    env.code_mem.emplace_back(0x4E703804);  // SADDLV s4, v0.8h
    env.code_mem.emplace_back(0x0EA02805);  // SADDLP v5.1d, v0.2s
    env.code_mem.emplace_back(0x4EB03806);  // SADDLV d6, v0.4s
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetVector(0, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF});
    jit.SetPC(0);

    env.ticks_left = 7;
    jit.Run();

    REQUIRE(jit.GetVector(1) == Vector{0x000000000000fff8, 0x0000000000000000});
    REQUIRE(jit.GetVector(2) == Vector{0x000000000000fff0, 0x0000000000000000});
    REQUIRE(jit.GetVector(3) == Vector{0x00000000fffffffc, 0x0000000000000000});
    REQUIRE(jit.GetVector(4) == Vector{0x00000000fffffff8, 0x0000000000000000});
    REQUIRE(jit.GetVector(5) == Vector{0xfffffffffffffffe, 0x0000000000000000});
    REQUIRE(jit.GetVector(6) == Vector{0xfffffffffffffffc, 0x0000000000000000});
}

TEST_CASE("A64: VQADD", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x6e210c02);  // UQADD v2.16b, v0.16b, v1.16b
    env.code_mem.emplace_back(0x4e210c03);  // SQADD v3.16b, v0.16b, v1.16b
    env.code_mem.emplace_back(0x6e610c04);  // UQADD v4.8h,  v0.8h,  v1.8h
    env.code_mem.emplace_back(0x4e610c05);  // SQADD v5.8h,  v0.8h,  v1.8h
    env.code_mem.emplace_back(0x6ea10c06);  // UQADD v6.4s,  v0.4s,  v1.4s
    env.code_mem.emplace_back(0x4ea10c07);  // SQADD v7.4s,  v0.4s,  v1.4s
    env.code_mem.emplace_back(0x6ee10c08);  // UQADD v8.2d,  v0.2d,  v1.2d
    env.code_mem.emplace_back(0x4ee10c09);  // SQADD v9.2d,  v0.2d,  v1.2d
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetVector(0, {0x7F7F7F7F7F7F7F7F, 0x7FFFFFFF7FFF7FFF});
    jit.SetVector(1, {0x8010FF00807F0000, 0x8000000080008000});
    jit.SetPC(0);

    env.ticks_left = 9;
    jit.Run();

    REQUIRE(jit.GetVector(2) == Vector{0xff8fff7ffffe7f7f, 0xffffffffffffffff});
    REQUIRE(jit.GetVector(3) == Vector{0xff7f7e7fff7f7f7f, 0xffffffffffffffff});
    REQUIRE(jit.GetVector(4) == Vector{0xff8ffffffffe7f7f, 0xffffffffffffffff});
    REQUIRE(jit.GetVector(5) == Vector{0xff8f7e7ffffe7f7f, 0xffffffffffffffff});
    REQUIRE(jit.GetVector(6) == Vector{0xff907e7ffffe7f7f, 0xffffffffffffffff});
    REQUIRE(jit.GetVector(7) == Vector{0xff907e7ffffe7f7f, 0xffffffffffffffff});
    REQUIRE(jit.GetVector(8) == Vector{0xff907e7ffffe7f7f, 0xffffffffffffffff});
    REQUIRE(jit.GetVector(9) == Vector{0xff907e7ffffe7f7f, 0xffffffffffffffff});
}

TEST_CASE("A64: VQSUB", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x6e212c02);  // UQSUB v2.16b, v0.16b, v1.16b
    env.code_mem.emplace_back(0x4e212c03);  // SQSUB v3.16b, v0.16b, v1.16b
    env.code_mem.emplace_back(0x6e612c04);  // UQSUB v4.8h,  v0.8h,  v1.8h
    env.code_mem.emplace_back(0x4e612c05);  // SQSUB v5.8h,  v0.8h,  v1.8h
    env.code_mem.emplace_back(0x6ea12c06);  // UQSUB v6.4s,  v0.4s,  v1.4s
    env.code_mem.emplace_back(0x4ea12c07);  // SQSUB v7.4s,  v0.4s,  v1.4s
    env.code_mem.emplace_back(0x6ee12c08);  // UQSUB v8.2d,  v0.2d,  v1.2d
    env.code_mem.emplace_back(0x4ee12c09);  // SQSUB v9.2d,  v0.2d,  v1.2d
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetVector(0, {0x8010FF00807F0000, 0x8000000080008000});
    jit.SetVector(1, {0x7F7F7F7F7F7F7F7F, 0x7FFFFFFF7FFF7FFF});
    jit.SetPC(0);

    env.ticks_left = 9;
    jit.Run();

    REQUIRE(jit.GetVector(2) == Vector{0x0100800001000000, 0x0100000001000100});
    REQUIRE(jit.GetVector(3) == Vector{0x8091808180008181, 0x8001010180018001});
    REQUIRE(jit.GetVector(4) == Vector{0x00917f8101000000, 0x0001000000010001});
    REQUIRE(jit.GetVector(5) == Vector{0x8000800080008081, 0x8000000180008000});
    REQUIRE(jit.GetVector(6) == Vector{0x00917f8100ff8081, 0x0000000100010001});
    REQUIRE(jit.GetVector(7) == Vector{0x8000000080000000, 0x8000000080000000});
    REQUIRE(jit.GetVector(8) == Vector{0x00917f8100ff8081, 0x0000000100010001});
    REQUIRE(jit.GetVector(9) == Vector{0x8000000000000000, 0x8000000000000000});
}

TEST_CASE("A64: REV", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0xdac00c00);  // REV X0, X0
    env.code_mem.emplace_back(0x5ac00821);  // REV W1, W1
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetRegister(0, 0xaabbccddeeff1100);
    jit.SetRegister(1, 0xaabbccdd);
    jit.SetPC(0);

    env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.GetRegister(0) == 0x11ffeeddccbbaa);
    REQUIRE(jit.GetRegister(1) == 0xddccbbaa);
    REQUIRE(jit.GetPC() == 8);
}

TEST_CASE("A64: REV32", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0xdac00800);  // REV32 X0, X0
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetRegister(0, 0xaabbccddeeff1100);
    jit.SetPC(0);

    env.ticks_left = 2;
    jit.Run();
    REQUIRE(jit.GetRegister(0) == 0xddccbbaa0011ffee);
    REQUIRE(jit.GetPC() == 4);
}

TEST_CASE("A64: REV16", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0xdac00400);  // REV16 X0, X0
    env.code_mem.emplace_back(0x5ac00421);  // REV16 W1, W1
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetRegister(0, 0xaabbccddeeff1100);
    jit.SetRegister(1, 0xaabbccdd);

    jit.SetPC(0);

    env.ticks_left = 3;
    jit.Run();
    REQUIRE(jit.GetRegister(0) == 0xbbaaddccffee0011);
    REQUIRE(jit.GetRegister(1) == 0xbbaaddcc);
    REQUIRE(jit.GetPC() == 8);
}

TEST_CASE("A64: SSHL", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4e204484);  // SSHL v4.16b, v4.16b, v0.16b
    env.code_mem.emplace_back(0x4e6144a5);  // SSHL  v5.8h,  v5.8h,  v1.8h
    env.code_mem.emplace_back(0x4ea244c6);  // SSHL  v6.4s,  v6.4s,  v2.4s
    env.code_mem.emplace_back(0x4ee344e7);  // SSHL  v7.2d,  v7.2d,  v3.2d
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0xEFF0FAFBFCFDFEFF, 0x0807050403020100});
    jit.SetVector(1, {0xFFFCFFFDFFFEFFFF, 0x0004000300020001});
    jit.SetVector(2, {0xFFFFFFFDFFFFFFFE, 0x0000000200000001});
    jit.SetVector(3, {0xFFFFFFFFFFFFFFFF, 0x0000000000000001});

    jit.SetVector(4, {0x8080808080808080, 0xFFFFFFFFFFFFFFFF});
    jit.SetVector(5, {0x8000800080008000, 0xFFFFFFFFFFFFFFFF});
    jit.SetVector(6, {0x8000000080000000, 0xFFFFFFFFFFFFFFFF});
    jit.SetVector(7, {0x8000000000000000, 0xFFFFFFFFFFFFFFFF});

    env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.GetVector(4) == Vector{0xfffffefcf8f0e0c0, 0x0080e0f0f8fcfeff});
    REQUIRE(jit.GetVector(5) == Vector{0xf800f000e000c000, 0xfff0fff8fffcfffe});
    REQUIRE(jit.GetVector(6) == Vector{0xf0000000e0000000, 0xfffffffcfffffffe});
    REQUIRE(jit.GetVector(7) == Vector{0xc000000000000000, 0xfffffffffffffffe});
}

TEST_CASE("A64: USHL", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x6e204484);  // USHL v4.16b, v4.16b, v0.16b
    env.code_mem.emplace_back(0x6e6144a5);  // USHL  v5.8h,  v5.8h,  v1.8h
    env.code_mem.emplace_back(0x6ea244c6);  // USHL  v6.4s,  v6.4s,  v2.4s
    env.code_mem.emplace_back(0x6ee344e7);  // USHL  v7.2d,  v7.2d,  v3.2d
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0x100F0E0D0C0B0A09, 0x0807050403020100});
    jit.SetVector(1, {0x0008000700060005, 0x0004000300020001});
    jit.SetVector(2, {0x0000000400000003, 0x0000000200000001});
    jit.SetVector(3, {0x0000000000000002, 0x0000000000000001});

    jit.SetVector(4, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF});
    jit.SetVector(5, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF});
    jit.SetVector(6, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF});
    jit.SetVector(7, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF});

    env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.GetVector(4) == Vector{0x0000000000000000, 0x0080e0f0f8fcfeff});
    REQUIRE(jit.GetVector(5) == Vector{0xff00ff80ffc0ffe0, 0xfff0fff8fffcfffe});
    REQUIRE(jit.GetVector(6) == Vector{0xfffffff0fffffff8, 0xfffffffcfffffffe});
    REQUIRE(jit.GetVector(7) == Vector{0xfffffffffffffffc, 0xfffffffffffffffe});
}

TEST_CASE("A64: XTN", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x0e212803);  // XTN v3.8b, v0.8h
    env.code_mem.emplace_back(0x0e612824);  // XTN v4.4h, v1.4s
    env.code_mem.emplace_back(0x0ea12845);  // XTN v5.2s, v2.2d
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0x3333222211110000, 0x7777666655554444});
    jit.SetVector(1, {0x1111111100000000, 0x3333333322222222});
    jit.SetVector(2, {0x0000000000000000, 0x1111111111111111});

    env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.GetVector(3) == Vector{0x7766554433221100, 0x0000000000000000});
    REQUIRE(jit.GetVector(4) == Vector{0x3333222211110000, 0x0000000000000000});
    REQUIRE(jit.GetVector(5) == Vector{0x1111111100000000, 0x0000000000000000});
}

TEST_CASE("A64: TBL", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x0e000100);  // TBL v0.8b,  { v8.16b                           }, v0.8b
    env.code_mem.emplace_back(0x4e010101);  // TBL v1.16b, { v8.16b                           }, v1.16b
    env.code_mem.emplace_back(0x0e022102);  // TBL v2.8b,  { v8.16b, v9.16b                   }, v2.8b
    env.code_mem.emplace_back(0x4e032103);  // TBL v3.16b, { v8.16b, v9.16b                   }, v3.16b
    env.code_mem.emplace_back(0x0e044104);  // TBL v4.8b,  { v8.16b, v9.16b, v10.16b          }, v4.8b
    env.code_mem.emplace_back(0x4e054105);  // TBL v5.16b, { v8.16b, v9.16b, v10.16b          }, v5.16b
    env.code_mem.emplace_back(0x0e066106);  // TBL v6.8b,  { v8.16b, v9.16b, v10.16b, v11.16b }, v6.8b
    env.code_mem.emplace_back(0x4e076107);  // TBL v7.16b, { v8.16b, v9.16b, v10.16b, v11.16b }, v7.16b
    env.code_mem.emplace_back(0x14000000);  // B .

    // Indices
    // 'FF' intended to test out-of-index
    jit.SetVector(0, {0x000102030405'FF'07, 0x08090a0b0c0d0e0f});
    jit.SetVector(1, {0x000102030405'FF'07, 0x08090a0b0c0d0e0f});
    jit.SetVector(2, {0x100011011202'FF'03, 0x1404150516061707});
    jit.SetVector(3, {0x100011011202'FF'03, 0x1404150516061707});
    jit.SetVector(4, {0x201000211101'FF'12, 0x0233231303241404});
    jit.SetVector(5, {0x201000211101'FF'12, 0x0233231303241404});
    jit.SetVector(6, {0x403010004131'FF'01, 0x4232120243332303});
    jit.SetVector(7, {0x403010004131'FF'01, 0x4232120243332303});

    // Table
    jit.SetVector(8, {0x7766554433221100, 0xffeeddccbbaa9988});
    jit.SetVector(9, {0xffffffffffffffff, 0xffffffffffffffff});
    jit.SetVector(10, {0xeeeeeeeeeeeeeeee, 0xeeeeeeeeeeeeeeee});
    jit.SetVector(11, {0xdddddddddddddddd, 0xdddddddddddddddd});

    jit.SetPC(0);

    env.ticks_left = 9;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x001122334455'00'77, 0x0000000000000000});
    REQUIRE(jit.GetVector(1) == Vector{0x001122334455'00'77, 0x8899aabbccddeeff});
    REQUIRE(jit.GetVector(2) == Vector{0xff00ff11ff22'00'33, 0x0000000000000000});
    REQUIRE(jit.GetVector(3) == Vector{0xff00ff11ff22'00'33, 0xff44ff55ff66ff77});
    REQUIRE(jit.GetVector(4) == Vector{0xeeff00eeff11'00'ff, 0x0000000000000000});
    REQUIRE(jit.GetVector(5) == Vector{0xeeff00eeff11'00'ff, 0x2200eeff33eeff44});
    REQUIRE(jit.GetVector(6) == Vector{0x00ddff0000dd'00'11, 0x0000000000000000});
    REQUIRE(jit.GetVector(7) == Vector{0x00ddff0000dd'00'11, 0x00ddff2200ddee33});
}

TEST_CASE("A64: TBX", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x0e001100);  // TBX v0.8b,  { v8.16b                           }, v0.8b
    env.code_mem.emplace_back(0x4e011101);  // TBX v1.16b, { v8.16b                           }, v1.16b
    env.code_mem.emplace_back(0x0e023102);  // TBX v2.8b,  { v8.16b, v9.16b                   }, v2.8b
    env.code_mem.emplace_back(0x4e033103);  // TBX v3.16b, { v8.16b, v9.16b                   }, v3.16b
    env.code_mem.emplace_back(0x0e045104);  // TBX v4.8b,  { v8.16b, v9.16b, v10.16b          }, v4.8b
    env.code_mem.emplace_back(0x4e055105);  // TBX v5.16b, { v8.16b, v9.16b, v10.16b          }, v5.16b
    env.code_mem.emplace_back(0x0e067106);  // TBX v6.8b,  { v8.16b, v9.16b, v10.16b, v11.16b }, v6.8b
    env.code_mem.emplace_back(0x4e077107);  // TBX v7.16b, { v8.16b, v9.16b, v10.16b, v11.16b }, v7.16b
    env.code_mem.emplace_back(0x14000000);  // B .

    // Indices
    // 'FF' intended to test out-of-index
    jit.SetVector(0, {0x000102030405'FF'07, 0x08090a0b0c0d0e0f});
    jit.SetVector(1, {0x000102030405'FF'07, 0x08090a0b0c0d0e0f});
    jit.SetVector(2, {0x100011011202'FF'03, 0x1404150516061707});
    jit.SetVector(3, {0x100011011202'FF'03, 0x1404150516061707});
    jit.SetVector(4, {0x201000211101'FF'12, 0x0233231303241404});
    jit.SetVector(5, {0x201000211101'FF'12, 0x0233231303241404});
    jit.SetVector(6, {0x403010004131'FF'01, 0x4232120243332303});
    jit.SetVector(7, {0x403010004131'FF'01, 0x4232120243332303});

    // Table
    jit.SetVector(8, {0x7766554433221100, 0xffeeddccbbaa9988});
    jit.SetVector(9, {0xffffffffffffffff, 0xffffffffffffffff});
    jit.SetVector(10, {0xeeeeeeeeeeeeeeee, 0xeeeeeeeeeeeeeeee});
    jit.SetVector(11, {0xdddddddddddddddd, 0xdddddddddddddddd});

    jit.SetPC(0);

    env.ticks_left = 9;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x001122334455'FF'77, 0x0000000000000000});
    REQUIRE(jit.GetVector(1) == Vector{0x001122334455'FF'77, 0x8899aabbccddeeff});
    REQUIRE(jit.GetVector(2) == Vector{0xff00ff11ff22'FF'33, 0x0000000000000000});
    REQUIRE(jit.GetVector(3) == Vector{0xff00ff11ff22'FF'33, 0xff44ff55ff66ff77});
    REQUIRE(jit.GetVector(4) == Vector{0xeeff00eeff11'FF'ff, 0x0000000000000000});
    REQUIRE(jit.GetVector(5) == Vector{0xeeff00eeff11'FF'ff, 0x2233eeff33eeff44});
    REQUIRE(jit.GetVector(6) == Vector{0x40ddff0041dd'FF'11, 0x0000000000000000});
    REQUIRE(jit.GetVector(7) == Vector{0x40ddff0041dd'FF'11, 0x42ddff2243ddee33});
}

TEST_CASE("A64: AND", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x8a020020);  // AND X0, X1, X2
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetRegister(0, 0);
    jit.SetRegister(1, 1);
    jit.SetRegister(2, 3);
    jit.SetPC(0);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetRegister(0) == 1);
    REQUIRE(jit.GetRegister(1) == 1);
    REQUIRE(jit.GetRegister(2) == 3);
    REQUIRE(jit.GetPC() == 4);
}

TEST_CASE("A64: Bitmasks", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x3200c3e0);  // ORR W0, WZR, #0x01010101
    env.code_mem.emplace_back(0x320c8fe1);  // ORR W1, WZR, #0x00F000F0
    env.code_mem.emplace_back(0x320003e2);  // ORR W2, WZR, #1
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);

    env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.GetRegister(0) == 0x01010101);
    REQUIRE(jit.GetRegister(1) == 0x00F000F0);
    REQUIRE(jit.GetRegister(2) == 1);
    REQUIRE(jit.GetPC() == 12);
}

TEST_CASE("A64: ANDS NZCV", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x6a020020);  // ANDS W0, W1, W2
    env.code_mem.emplace_back(0x14000000);  // B .

    SECTION("N=1, Z=0") {
        jit.SetRegister(0, 0);
        jit.SetRegister(1, 0xFFFFFFFF);
        jit.SetRegister(2, 0xFFFFFFFF);
        jit.SetPC(0);

        env.ticks_left = 2;
        jit.Run();

        REQUIRE(jit.GetRegister(0) == 0xFFFFFFFF);
        REQUIRE(jit.GetRegister(1) == 0xFFFFFFFF);
        REQUIRE(jit.GetRegister(2) == 0xFFFFFFFF);
        REQUIRE(jit.GetPC() == 4);
        REQUIRE((jit.GetPstate() & 0xF0000000) == 0x80000000);
    }

    SECTION("N=0, Z=1") {
        jit.SetRegister(0, 0);
        jit.SetRegister(1, 0xFFFFFFFF);
        jit.SetRegister(2, 0x00000000);
        jit.SetPC(0);

        env.ticks_left = 2;
        jit.Run();

        REQUIRE(jit.GetRegister(0) == 0x00000000);
        REQUIRE(jit.GetRegister(1) == 0xFFFFFFFF);
        REQUIRE(jit.GetRegister(2) == 0x00000000);
        REQUIRE(jit.GetPC() == 4);
        REQUIRE((jit.GetPstate() & 0xF0000000) == 0x40000000);
    }
    SECTION("N=0, Z=0") {
        jit.SetRegister(0, 0);
        jit.SetRegister(1, 0x12345678);
        jit.SetRegister(2, 0x7324a993);
        jit.SetPC(0);

        env.ticks_left = 2;
        jit.Run();

        REQUIRE(jit.GetRegister(0) == 0x12240010);
        REQUIRE(jit.GetRegister(1) == 0x12345678);
        REQUIRE(jit.GetRegister(2) == 0x7324a993);
        REQUIRE(jit.GetPC() == 4);
        REQUIRE((jit.GetPstate() & 0xF0000000) == 0x00000000);
    }
}

TEST_CASE("A64: CBZ", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x34000060);  // 0x00 : CBZ X0, label
    env.code_mem.emplace_back(0x320003e2);  // 0x04 : MOV X2, 1
    env.code_mem.emplace_back(0x14000000);  // 0x08 : B.
    env.code_mem.emplace_back(0x321f03e2);  // 0x0C : label: MOV X2, 2
    env.code_mem.emplace_back(0x14000000);  // 0x10 : B .

    SECTION("no branch") {
        jit.SetPC(0);
        jit.SetRegister(0, 1);

        env.ticks_left = 4;
        jit.Run();

        REQUIRE(jit.GetRegister(2) == 1);
        REQUIRE(jit.GetPC() == 8);
    }

    SECTION("branch") {
        jit.SetPC(0);
        jit.SetRegister(0, 0);

        env.ticks_left = 4;
        jit.Run();

        REQUIRE(jit.GetRegister(2) == 2);
        REQUIRE(jit.GetPC() == 16);
    }
}

TEST_CASE("A64: TBZ", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x36180060);  // 0x00 : TBZ X0, 3, label
    env.code_mem.emplace_back(0x320003e2);  // 0x04 : MOV X2, 1
    env.code_mem.emplace_back(0x14000000);  // 0x08 : B .
    env.code_mem.emplace_back(0x321f03e2);  // 0x0C : label: MOV X2, 2
    env.code_mem.emplace_back(0x14000000);  // 0x10 : B .

    SECTION("no branch") {
        jit.SetPC(0);
        jit.SetRegister(0, 0xFF);

        env.ticks_left = 4;
        jit.Run();

        REQUIRE(jit.GetRegister(2) == 1);
        REQUIRE(jit.GetPC() == 8);
    }

    SECTION("branch with zero") {
        jit.SetPC(0);
        jit.SetRegister(0, 0);

        env.ticks_left = 4;
        jit.Run();

        REQUIRE(jit.GetRegister(2) == 2);
        REQUIRE(jit.GetPC() == 16);
    }

    SECTION("branch with non-zero") {
        jit.SetPC(0);
        jit.SetRegister(0, 1);

        env.ticks_left = 4;
        jit.Run();

        REQUIRE(jit.GetRegister(2) == 2);
        REQUIRE(jit.GetPC() == 16);
    }
}

TEST_CASE("A64: FABD", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x6eb5d556);  // FABD.4S V22, V10, V21
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(10, {0xb4858ac77ff39a87, 0x9fce5e14c4873176});
    jit.SetVector(21, {0x56d3f085ff890e2b, 0x6e4b0a41801a2d00});

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(22) == Vector{0x56d3f0857fc90e2b, 0x6e4b0a4144873176});
}

TEST_CASE("A64: FABS", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4ef8f804);  // FABS v4.8h, v0.8h
    env.code_mem.emplace_back(0x4ea0f825);  // FABS v5.4s, v1.4s
    env.code_mem.emplace_back(0x4ee0f846);  // FABS v6.2d, v2.2d
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0xffffffffffffffff, 0xffffffffffff8000});
    jit.SetVector(1, {0xffbfffffffc00000, 0xff80000080000000});
    jit.SetVector(2, {0xffffffffffffffff, 0x8000000000000000});

    env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.GetVector(4) == Vector{0x7fff7fff7fff7fff, 0x7fff7fff7fff0000});
    REQUIRE(jit.GetVector(5) == Vector{0x7fbfffff7fc00000, 0x7f80000000000000});
    REQUIRE(jit.GetVector(6) == Vector{0x7fffffffffffffff, 0x0000000000000000});
}

TEST_CASE("A64: FMIN", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4ea1f400);  // FMIN.4S V0, V0, V1
    env.code_mem.emplace_back(0x4ee3f442);  // FMIN.2D V2, V2, V3
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0x7fc00000'09503366, 0x00000000'7f984a37});
    jit.SetVector(1, {0xc1200000'00000001, 0x6e4b0a41'ffffffff});

    jit.SetVector(2, {0x7fc0000009503366, 0x3ff0000000000000});
    jit.SetVector(3, {0xbff0000000000000, 0x6e4b0a41ffffffff});

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x7fc00000'00000001, 0x00000000'7fd84a37});
    REQUIRE(jit.GetVector(2) == Vector{0xbff0000000000000, 0x3ff0000000000000});
}

TEST_CASE("A64: FMAX", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4e21f400);  // FMAX.4S V0, V0, V1
    env.code_mem.emplace_back(0x4e63f442);  // FMAX.2D V2, V2, V3
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0x7fc00000'09503366, 0x00000000'7f984a37});
    jit.SetVector(1, {0xc1200000'00000001, 0x6e4b0a41'ffffffff});

    jit.SetVector(2, {0x7fc0000009503366, 0x3ff0000000000000});
    jit.SetVector(3, {0xbff0000000000000, 0x6e4b0a41ffffffff});

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x7fc00000'09503366, 0x6e4b0a41'7fd84a37});
    REQUIRE(jit.GetVector(2) == Vector{0x7fc0000009503366, 0x6e4b0a41ffffffff});
}

TEST_CASE("A64: FMINNM", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4ea1c400);  // FMINNM.4S V0, V0, V1
    env.code_mem.emplace_back(0x4ee3c442);  // FMINNM.2D V2, V2, V3
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0x7fc00000'09503366, 0x00000000'7f984a37});
    jit.SetVector(1, {0xc1200000'00000001, 0x6e4b0a41'ffffffff});

    jit.SetVector(2, {0x7fc0000009503366, 0x3ff0000000000000});
    jit.SetVector(3, {0xfff0000000000000, 0xffffffffffffffff});

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0xc1200000'00000001, 0x00000000'7fd84a37});
    REQUIRE(jit.GetVector(2) == Vector{0xfff0000000000000, 0x3ff0000000000000});
}

TEST_CASE("A64: FMAXNM", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4e21c400);  // FMAXNM.4S V0, V0, V1
    env.code_mem.emplace_back(0x4e63c442);  // FMAXNM.2D V2, V2, V3
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0x7fc00000'09503366, 0x00000000'7f984a37});
    jit.SetVector(1, {0xc1200000'00000001, 0x6e4b0a41'ffffffff});

    jit.SetVector(2, {0x7fc0000009503366, 0x3ff0000000000000});
    jit.SetVector(3, {0xfff0000000000000, 0xffffffffffffffff});

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0xc1200000'09503366, 0x6e4b0a41'7fd84a37});
    REQUIRE(jit.GetVector(2) == Vector{0x7fc0000009503366, 0x3ff0000000000000});
}

TEST_CASE("A64: 128-bit exclusive read/write", "[a64]") {
    A64TestEnv env;
    ExclusiveMonitor monitor{1};

    A64::UserConfig conf;
    conf.callbacks = &env;
    conf.processor_id = 0;

    SECTION("Global Monitor") {
        conf.global_monitor = &monitor;
    }

    A64::Jit jit{conf};

    env.code_mem.emplace_back(0xc87f0861);  // LDXP X1, X2, [X3]
    env.code_mem.emplace_back(0xc8241865);  // STXP W4, X5, X6, [X3]
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetRegister(3, 0x1234567812345678);
    jit.SetRegister(4, 0xbaadbaadbaadbaad);
    jit.SetRegister(5, 0xaf00d1e5badcafe0);
    jit.SetRegister(6, 0xd0d0cacad0d0caca);

    env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.GetRegister(1) == 0x7f7e7d7c7b7a7978);
    REQUIRE(jit.GetRegister(2) == 0x8786858483828180);
    REQUIRE(jit.GetRegister(4) == 0);
    REQUIRE(env.MemoryRead64(0x1234567812345678) == 0xaf00d1e5badcafe0);
    REQUIRE(env.MemoryRead64(0x1234567812345680) == 0xd0d0cacad0d0caca);
}

TEST_CASE("A64: CNTPCT_EL0", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0xd53be021);  // MRS X1, CNTPCT_EL0
    env.code_mem.emplace_back(0xd503201f);  // NOP
    env.code_mem.emplace_back(0xd503201f);  // NOP
    env.code_mem.emplace_back(0xd503201f);  // NOP
    env.code_mem.emplace_back(0xd503201f);  // NOP
    env.code_mem.emplace_back(0xd503201f);  // NOP
    env.code_mem.emplace_back(0xd503201f);  // NOP
    env.code_mem.emplace_back(0xd53be022);  // MRS X2, CNTPCT_EL0
    env.code_mem.emplace_back(0xcb010043);  // SUB X3, X2, X1
    env.code_mem.emplace_back(0x14000000);  // B .

    env.ticks_left = 10;
    jit.Run();

    REQUIRE(jit.GetRegister(3) == 7);
}

TEST_CASE("A64: FNMSUB 1", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x1f618a9c);  // FNMSUB D28, D20, D1, D2
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(20, {0xe73a51346164bd6c, 0x8080000000002b94});
    jit.SetVector(1, {0xbf8000007fffffff, 0xffffffff00002b94});
    jit.SetVector(2, {0x0000000000000000, 0xc79b271e3f000000});

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(28) == Vector{0x66ca513533ee6076, 0x0000000000000000});
}

TEST_CASE("A64: FNMSUB 2", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x1f2ab88e);  // FNMSUB S14, S4, S10, S14
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(4, {0x3c9623b101398437, 0x7ff0abcd0ba98d27});
    jit.SetVector(10, {0xffbfffff3eaaaaab, 0x3f0000003f8147ae});
    jit.SetVector(14, {0x80000000007fffff, 0xe73a513400000000});
    jit.SetFpcr(0x00400000);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(14) == Vector{0x0000000080045284, 0x0000000000000000});
}

TEST_CASE("A64: FMADD", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x1f5e0e4a);  // FMADD D10, D18, D30, D3
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(18, {0x8000007600800000, 0x7ff812347f800000});
    jit.SetVector(30, {0xff984a3700000000, 0xe73a513480800000});
    jit.SetVector(3, {0x3f000000ff7fffff, 0x8139843780000000});
    jit.SetFpcr(0x00400000);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(10) == Vector{0x3f059921bf0dbfff, 0x0000000000000000});
}

TEST_CASE("A64: FMLA.4S(lane)", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4f8f11c0);  // FMLA.4S V0, V14, V15[0]
    env.code_mem.emplace_back(0x4faf11c1);  // FMLA.4S V1, V14, V15[1]
    env.code_mem.emplace_back(0x4f8f19c2);  // FMLA.4S V2, V14, V15[2]
    env.code_mem.emplace_back(0x4faf19c3);  // FMLA.4S V3, V14, V15[3]
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(0, {0x3ff00000'3ff00000, 0x00000000'00000000});
    jit.SetVector(1, {0x3ff00000'3ff00000, 0x00000000'00000000});
    jit.SetVector(2, {0x3ff00000'3ff00000, 0x00000000'00000000});
    jit.SetVector(3, {0x3ff00000'3ff00000, 0x00000000'00000000});

    jit.SetVector(14, {0x3ff00000'3ff00000, 0x3ff00000'3ff00000});
    jit.SetVector(15, {0x3ff00000'40000000, 0x40400000'40800000});

    env.ticks_left = 5;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x40b4000040b40000, 0x4070000040700000});
    REQUIRE(jit.GetVector(1) == Vector{0x40ac800040ac8000, 0x4061000040610000});
    REQUIRE(jit.GetVector(2) == Vector{0x4116000041160000, 0x40f0000040f00000});
    REQUIRE(jit.GetVector(3) == Vector{0x40f0000040f00000, 0x40b4000040b40000});
}

TEST_CASE("A64: FMUL.4S(lane)", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4f8f91c0);  // FMUL.4S V0, V14, V15[0]
    env.code_mem.emplace_back(0x4faf91c1);  // FMUL.4S V1, V14, V15[1]
    env.code_mem.emplace_back(0x4f8f99c2);  // FMUL.4S V2, V14, V15[2]
    env.code_mem.emplace_back(0x4faf99c3);  // FMUL.4S V3, V14, V15[3]
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(14, {0x3ff00000'3ff00000, 0x3ff00000'3ff00000});
    jit.SetVector(15, {0x3ff00000'40000000, 0x40400000'40800000});

    env.ticks_left = 5;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x4070000040700000, 0x4070000040700000});
    REQUIRE(jit.GetVector(1) == Vector{0x4061000040610000, 0x4061000040610000});
    REQUIRE(jit.GetVector(2) == Vector{0x40f0000040f00000, 0x40f0000040f00000});
    REQUIRE(jit.GetVector(3) == Vector{0x40b4000040b40000, 0x40b4000040b40000});
}

TEST_CASE("A64: FMLA.4S (denormal)", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4e2fcccc);  // FMLA.4S V12, V6, V15
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(12, {0x3c9623b17ff80000, 0xbff0000080000076});
    jit.SetVector(6, {0x7ff80000ff800000, 0x09503366c1200000});
    jit.SetVector(15, {0x3ff0000080636d24, 0xbf800000e73a5134});
    jit.SetFpcr(0x01000000);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(12) == Vector{0x7ff800007fc00000, 0xbff0000068e8e581});
}

TEST_CASE("A64: FMLA.4S (0x80800000)", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4e38cc2b);  // FMLA.4S V11, V1, V24
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(11, {0xc79b271efff05678, 0xffc0000080800000});
    jit.SetVector(1, {0x00636d2400800000, 0x0966320bb26bddee});
    jit.SetVector(24, {0x460e8c84fff00000, 0x8ba98d2780800002});
    jit.SetFpcr(0x03000000);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(11) == Vector{0xc79b271e7fc00000, 0x7fc0000080000000});
}

// x64 has different rounding behaviour to AArch64.
// AArch64 performs rounding after flushing-to-zero.
// x64 performs rounding before flushing-to-zero.
TEST_CASE("A64: FMADD (0x80800000)", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x1f0f7319);  // FMADD S25, S24, S15, S28
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(24, {0x00800000, 0});
    jit.SetVector(15, {0x0ba98d27, 0});
    jit.SetVector(28, {0x80800000, 0});
    jit.SetFpcr(0x01000000);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(25) == Vector{0x80000000, 0});
}

TEST_CASE("A64: FNEG failed to zero upper", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x2ea0fb50);  // FNEG.2S V16, V26
    env.code_mem.emplace_back(0x2e207a1c);  // SQNEG.8B V28, V16
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(26, {0x071286fde8f34a90, 0x837cffa8be382f60});
    jit.SetFpcr(0x01000000);

    env.ticks_left = 6;
    jit.Run();

    REQUIRE(jit.GetVector(28) == Vector{0x79ee7a03980db670, 0});
    REQUIRE(FP::FPSR{jit.GetFpsr()}.QC() == false);
}

TEST_CASE("A64: FRSQRTS", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x5eb8fcad);  // FRSQRTS S13, S5, S24
    env.code_mem.emplace_back(0x14000000);  // B .

    // These particular values result in an intermediate value during
    // the calculation that is close to infinity. We want to verify
    // that this special case is handled appropriately.

    jit.SetPC(0);
    jit.SetVector(5, {0xfc6a0206, 0});
    jit.SetVector(24, {0xfc6a0206, 0});
    jit.SetFpcr(0x00400000);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(13) == Vector{0xff7fffff, 0});
}

TEST_CASE("A64: SQDMULH.8H (saturate)", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4e62b420);  // SQDMULH.8H V0, V1, V2
    env.code_mem.emplace_back(0x14000000);  // B .

    // Make sure that saturating values are tested

    jit.SetPC(0);
    jit.SetVector(1, {0x7fff80007ffe8001, 0x7fff80007ffe8001});
    jit.SetVector(2, {0x7fff80007ffe8001, 0x80007fff80017ffe});
    jit.SetFpsr(0);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x7ffe7fff7ffc7ffe, 0x8001800180028002});
    REQUIRE(FP::FPSR{jit.GetFpsr()}.QC() == true);
}

TEST_CASE("A64: SQDMULH.4S (saturate)", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x4ea2b420);  // SQDMULH.4S V0, V1, V2
    env.code_mem.emplace_back(0x14000000);  // B .

    // Make sure that saturating values are tested

    jit.SetPC(0);
    jit.SetVector(1, {0x7fffffff80000000, 0x7fffffff80000000});
    jit.SetVector(2, {0x7fffffff80000000, 0x800000007fffffff});
    jit.SetFpsr(0);

    env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.GetVector(0) == Vector{0x7ffffffe7fffffff, 0x8000000180000001});
    REQUIRE(FP::FPSR{jit.GetFpsr()}.QC() == true);
}

TEST_CASE("A64: This is an infinite loop if fast dispatch is enabled", "[a64]") {
    A64TestEnv env;
    A64::UserConfig conf{&env};
    conf.optimizations &= ~OptimizationFlag::FastDispatch;
    A64::Jit jit{conf};

    env.code_mem.emplace_back(0x2ef998fa);
    env.code_mem.emplace_back(0x2ef41c11);
    env.code_mem.emplace_back(0x0f07fdd8);
    env.code_mem.emplace_back(0x9ac90d09);
    env.code_mem.emplace_back(0xd63f0120);  // BLR X9
    env.code_mem.emplace_back(0x14000000);  // B .

    env.ticks_left = 6;
    jit.Run();
}

TEST_CASE("A64: Optimization failure when folding ADD", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0xbc4f84be);  // LDR S30, [X5], #248
    env.code_mem.emplace_back(0x9a0c00ea);  // ADC X10, X7, X12
    env.code_mem.emplace_back(0x5a1a0079);  // SBC W25, W3, W26
    env.code_mem.emplace_back(0x9b0e2be9);  // MADD X9, XZR, X14, X10
    env.code_mem.emplace_back(0xfa5fe8a9);  // CCMP X5, #31, #9, AL
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetRegister(0, 0x46e15845dba57924);
    jit.SetRegister(1, 0x6f60d04350581fea);
    jit.SetRegister(2, 0x85cface50edcfc03);
    jit.SetRegister(3, 0x47e1e8906e10ec5a);
    jit.SetRegister(4, 0x70717c9450b6b707);
    jit.SetRegister(5, 0x300d83205baeaff4);
    jit.SetRegister(6, 0xb7890de7c6fee082);
    jit.SetRegister(7, 0xa89fb6d6f1b42f4a);
    jit.SetRegister(8, 0x04e36b8aada91d4f);
    jit.SetRegister(9, 0xa03bf6bde71c6ac5);
    jit.SetRegister(10, 0x319374d14baa83b0);
    jit.SetRegister(11, 0x5a78fc0fffca7c5f);
    jit.SetRegister(12, 0xc012b5063f43b8ad);
    jit.SetRegister(13, 0x821ade159d39fea1);
    jit.SetRegister(14, 0x41f97b2f5525c25e);
    jit.SetRegister(15, 0xab0cd3653cb93738);
    jit.SetRegister(16, 0x50dfcb55a4ebd554);
    jit.SetRegister(17, 0x30dd7d18ae52df03);
    jit.SetRegister(18, 0x4e53b20d252bf085);
    jit.SetRegister(19, 0x013582d71f5fd42a);
    jit.SetRegister(20, 0x97a151539dad44e7);
    jit.SetRegister(21, 0xa6fcc6bb220a2ad3);
    jit.SetRegister(22, 0x4c84d3c84a6c5c5c);
    jit.SetRegister(23, 0x1a7596a5ef930dff);
    jit.SetRegister(24, 0x06248d96a02ff210);
    jit.SetRegister(25, 0xfcb8772aec4b1dfd);
    jit.SetRegister(26, 0x63619787b6a17665);
    jit.SetRegister(27, 0xbd50c3352d001e40);
    jit.SetRegister(28, 0x4e186aae63c81553);
    jit.SetRegister(29, 0x57462b7163bd6508);
    jit.SetRegister(30, 0xa977c850d16d562c);
    jit.SetSP(0x000000da9b761d8c);
    jit.SetFpsr(0x03480000);
    jit.SetPstate(0x30000000);

    env.ticks_left = 6;
    jit.Run();

    REQUIRE(jit.GetRegister(0) == 0x46e15845dba57924);
    REQUIRE(jit.GetRegister(1) == 0x6f60d04350581fea);
    REQUIRE(jit.GetRegister(2) == 0x85cface50edcfc03);
    REQUIRE(jit.GetRegister(3) == 0x47e1e8906e10ec5a);
    REQUIRE(jit.GetRegister(4) == 0x70717c9450b6b707);
    REQUIRE(jit.GetRegister(5) == 0x300d83205baeb0ec);
    REQUIRE(jit.GetRegister(6) == 0xb7890de7c6fee082);
    REQUIRE(jit.GetRegister(7) == 0xa89fb6d6f1b42f4a);
    REQUIRE(jit.GetRegister(8) == 0x04e36b8aada91d4f);
    REQUIRE(jit.GetRegister(9) == 0x68b26bdd30f7e7f8);
    REQUIRE(jit.GetRegister(10) == 0x68b26bdd30f7e7f8);
    REQUIRE(jit.GetRegister(11) == 0x5a78fc0fffca7c5f);
    REQUIRE(jit.GetRegister(12) == 0xc012b5063f43b8ad);
    REQUIRE(jit.GetRegister(13) == 0x821ade159d39fea1);
    REQUIRE(jit.GetRegister(14) == 0x41f97b2f5525c25e);
    REQUIRE(jit.GetRegister(15) == 0xab0cd3653cb93738);
    REQUIRE(jit.GetRegister(16) == 0x50dfcb55a4ebd554);
    REQUIRE(jit.GetRegister(17) == 0x30dd7d18ae52df03);
    REQUIRE(jit.GetRegister(18) == 0x4e53b20d252bf085);
    REQUIRE(jit.GetRegister(19) == 0x013582d71f5fd42a);
    REQUIRE(jit.GetRegister(20) == 0x97a151539dad44e7);
    REQUIRE(jit.GetRegister(21) == 0xa6fcc6bb220a2ad3);
    REQUIRE(jit.GetRegister(22) == 0x4c84d3c84a6c5c5c);
    REQUIRE(jit.GetRegister(23) == 0x1a7596a5ef930dff);
    REQUIRE(jit.GetRegister(24) == 0x06248d96a02ff210);
    REQUIRE(jit.GetRegister(25) == 0x00000000b76f75f5);
    REQUIRE(jit.GetRegister(26) == 0x63619787b6a17665);
    REQUIRE(jit.GetRegister(27) == 0xbd50c3352d001e40);
    REQUIRE(jit.GetRegister(28) == 0x4e186aae63c81553);
    REQUIRE(jit.GetRegister(29) == 0x57462b7163bd6508);
    REQUIRE(jit.GetRegister(30) == 0xa977c850d16d562c);
    REQUIRE(jit.GetPstate() == 0x20000000);
    REQUIRE(jit.GetVector(30) == Vector{0xf7f6f5f4, 0});
}

TEST_CASE("A64: Cache Maintenance Instructions", "[a64]") {
    class CacheMaintenanceTestEnv final : public A64TestEnv {
        void InstructionCacheOperationRaised(A64::InstructionCacheOperation op, VAddr value) override {
            REQUIRE(op == A64::InstructionCacheOperation::InvalidateByVAToPoU);
            REQUIRE(value == 0xcafed00d);
        }
        void DataCacheOperationRaised(A64::DataCacheOperation op, VAddr value) override {
            REQUIRE(op == A64::DataCacheOperation::InvalidateByVAToPoC);
            REQUIRE(value == 0xcafebabe);
        }
    };

    CacheMaintenanceTestEnv env;
    A64::UserConfig conf{&env};
    conf.hook_data_cache_operations = true;
    A64::Jit jit{conf};

    jit.SetRegister(0, 0xcafed00d);
    jit.SetRegister(1, 0xcafebabe);

    env.code_mem.emplace_back(0xd50b7520);  // ic ivau, x0
    env.code_mem.emplace_back(0xd5087621);  // dc ivac, x1
    env.code_mem.emplace_back(0x14000000);  // B .

    env.ticks_left = 3;
    jit.Run();
}

TEST_CASE("A64: Memory access (fastmem)", "[a64]") {
    constexpr size_t address_width = 12;
    constexpr size_t memory_size = 1ull << address_width;  // 4K
    constexpr size_t page_size = 4 * 1024;
    constexpr size_t buffer_size = 2 * page_size;
    char buffer[buffer_size];

    void* buffer_ptr = reinterpret_cast<void*>(buffer);
    size_t buffer_size_nconst = buffer_size;
    char* backing_memory = reinterpret_cast<char*>(std::align(page_size, memory_size, buffer_ptr, buffer_size_nconst));

    A64FastmemTestEnv env{backing_memory};
    Dynarmic::A64::UserConfig config{&env};
    config.fastmem_pointer = backing_memory;
    config.fastmem_address_space_bits = address_width;
    config.recompile_on_fastmem_failure = false;
    config.silently_mirror_fastmem = true;
    config.processor_id = 0;

    Dynarmic::A64::Jit jit{config};
    memset(backing_memory, 0, memory_size);
    memcpy(backing_memory + 0x100, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.", 57);

    env.MemoryWrite32(0, 0xA9401404);   // LDP X4, X5, [X0]
    env.MemoryWrite32(4, 0xF9400046);   // LDR X6, [X2]
    env.MemoryWrite32(8, 0xA9001424);   // STP X4, X5, [X1]
    env.MemoryWrite32(12, 0xF9000066);  // STR X6, [X3]
    env.MemoryWrite32(16, 0x14000000);  // B .
    jit.SetRegister(0, 0x100);
    jit.SetRegister(1, 0x1F0);
    jit.SetRegister(2, 0x10F);
    jit.SetRegister(3, 0x1FF);

    jit.SetPC(0);
    jit.SetSP(memory_size - 1);
    jit.SetFpsr(0x03480000);
    jit.SetPstate(0x30000000);
    env.ticks_left = 5;

    jit.Run();
    REQUIRE(strncmp(backing_memory + 0x100, backing_memory + 0x1F0, 23) == 0);
}

TEST_CASE("A64: SQRDMULH QC flag when output invalidated", "[a64]") {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    env.code_mem.emplace_back(0x0fbcd38b);  // SQRDMULH.2S V11, V28, V28[1]
    env.code_mem.emplace_back(0x7ef0f8eb);  // FMINP.2D    D11, V7
    env.code_mem.emplace_back(0x14000000);  // B .

    jit.SetPC(0);
    jit.SetVector(7, {0xb1b5'd0b1'4e54'e281, 0xb4cb'4fec'8563'1032});
    jit.SetVector(28, {0x8000'0000'0000'0000, 0x0000'0000'0000'0000});
    jit.SetFpcr(0x05400000);

    env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.GetFpsr() == 0x08000000);
    REQUIRE(jit.GetVector(11) == Vector{0xb4cb'4fec'8563'1032, 0x0000'0000'0000'0000});
}
