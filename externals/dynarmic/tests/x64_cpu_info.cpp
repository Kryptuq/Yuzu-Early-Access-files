/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>
#include <utility>

#include <catch2/catch.hpp>
#include <xbyak/xbyak_util.h>

TEST_CASE("Host CPU supports", "[a64]") {
    using Cpu = Xbyak::util::Cpu;
    Cpu cpu_info;

    std::array<uint32_t, 4> cpu_name;
    for (uint32_t i = 2; i < 5; ++i) {
        cpu_info.getCpuid(0x80000000 | i, cpu_name.data());
        std::printf("%.16s", reinterpret_cast<const char*>(cpu_name.data()));
    }
    std::putchar('\n');

    cpu_info.putFamily();
    const std::array types{
#define X(NAME) std::make_pair(Cpu::Type{Cpu::NAME}, &#NAME[1])
        X(t3DN),
        X(tADX),
        X(tAESNI),
        X(tAMD),
        X(tAMX_BF16),
        X(tAMX_INT8),
        X(tAMX_TILE),
        X(tAVX),
        X(tAVX2),
        X(tAVX512_4FMAPS),
        X(tAVX512_4VNNIW),
        X(tAVX512_BF16),
        X(tAVX512_BITALG),
        X(tAVX512_FP16),
        X(tAVX512_IFMA),
        X(tAVX512_VBMI),
        X(tAVX512_VBMI2),
        X(tAVX512_VNNI),
        X(tAVX512_VP2INTERSECT),
        X(tAVX512_VPOPCNTDQ),
        X(tAVX512BW),
        X(tAVX512CD),
        X(tAVX512DQ),
        X(tAVX512ER),
        X(tAVX512F),
        X(tAVX512IFMA),
        X(tAVX512PF),
        X(tAVX512VBMI),
        X(tAVX512VL),
        X(tAVX_VNNI),
        X(tBMI1),
        X(tBMI2),
        X(tCLDEMOTE),
        X(tCLFLUSHOPT),
        X(tCLZERO),
        X(tCMOV),
        X(tE3DN),
        X(tENHANCED_REP),
        X(tF16C),
        X(tFMA),
        X(tGFNI),
        X(tHLE),
        X(tINTEL),
        X(tLZCNT),
        X(tMMX),
        X(tMMX2),
        X(tMOVBE),
        X(tMOVDIR64B),
        X(tMOVDIRI),
        X(tMPX),
        X(tOSXSAVE),
        X(tPCLMULQDQ),
        X(tPOPCNT),
        X(tPREFETCHW),
        X(tPREFETCHWT1),
        X(tRDRAND),
        X(tRDSEED),
        X(tRDTSCP),
        X(tRTM),
        X(tSHA),
        X(tSMAP),
        X(tSSE),
        X(tSSE2),
        X(tSSE3),
        X(tSSE41),
        X(tSSE42),
        X(tSSSE3),
        X(tVAES),
        X(tVPCLMULQDQ),
        X(tWAITPKG),
#undef X
    };

    constexpr size_t line_max = 80;
    size_t line_length = 0;
    for (const auto& [type, name] : types) {
        if (cpu_info.has(type)) {
            const size_t name_length = std::strlen(name) + 1;
            if ((line_length + name_length) >= line_max) {
                line_length = name_length;
                std::putchar('\n');
            } else if (line_length) {
                std::putchar(' ');
            }
            std::fputs(name, stdout);
            line_length += name_length;
        }
    }
    std::putchar('\n');
}
