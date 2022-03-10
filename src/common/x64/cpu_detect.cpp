// Copyright 2013 Dolphin Emulator Project / 2015 Citra Emulator Project / 2022 Yuzu Emulator
// Project Licensed under GPLv2 or any later version Refer to the license.txt file included.

#include <array>
#include <cstring>
#include <iterator>
#include <span>
#include <string_view>
#include "common/bit_util.h"
#include "common/common_types.h"
#include "common/x64/cpu_detect.h"

#ifdef _MSC_VER
#include <intrin.h>
#else

#if defined(__DragonFly__) || defined(__FreeBSD__)
// clang-format off
#include <sys/types.h>
#include <machine/cpufunc.h>
// clang-format on
#endif

static inline void __cpuidex(int info[4], u32 function_id, u32 subfunction_id) {
#if defined(__DragonFly__) || defined(__FreeBSD__)
    // Despite the name, this is just do_cpuid() with ECX as second input.
    cpuid_count((u_int)function_id, (u_int)subfunction_id, (u_int*)info);
#else
    info[0] = function_id;    // eax
    info[2] = subfunction_id; // ecx
    __asm__("cpuid"
            : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3])
            : "a"(function_id), "c"(subfunction_id));
#endif
}

static inline void __cpuid(int info[4], u32 function_id) {
    return __cpuidex(info, function_id, 0);
}

#define _XCR_XFEATURE_ENABLED_MASK 0
static inline u64 _xgetbv(u32 index) {
    u32 eax, edx;
    __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
    return ((u64)edx << 32) | eax;
}

#endif // _MSC_VER

namespace Common {

CPUCaps::Manufacturer CPUCaps::ParseManufacturer(std::string_view brand_string) {
    if (brand_string == "GenuineIntel") {
        return Manufacturer::Intel;
    } else if (brand_string == "AuthenticAMD") {
        return Manufacturer::AMD;
    } else if (brand_string == "HygonGenuine") {
        return Manufacturer::Hygon;
    }
    return Manufacturer::Unknown;
}

// Detects the various CPU features
static CPUCaps Detect() {
    CPUCaps caps = {};

    // Assumes the CPU supports the CPUID instruction. Those that don't would likely not support
    // yuzu at all anyway

    int cpu_id[4];

    // Detect CPU's CPUID capabilities and grab manufacturer string
    __cpuid(cpu_id, 0x00000000);
    const u32 max_std_fn = cpu_id[0]; // EAX

    std::memset(caps.brand_string, 0, std::size(caps.brand_string));
    std::memcpy(&caps.brand_string[0], &cpu_id[1], sizeof(u32));
    std::memcpy(&caps.brand_string[4], &cpu_id[3], sizeof(u32));
    std::memcpy(&caps.brand_string[8], &cpu_id[2], sizeof(u32));

    caps.manufacturer = CPUCaps::ParseManufacturer(caps.brand_string);

    // Set reasonable default cpu string even if brand string not available
    std::strncpy(caps.cpu_string, caps.brand_string, std::size(caps.brand_string));

    __cpuid(cpu_id, 0x80000000);

    const u32 max_ex_fn = cpu_id[0];

    // Detect family and other miscellaneous features
    if (max_std_fn >= 1) {
        __cpuid(cpu_id, 0x00000001);
        caps.sse = Common::Bit<25>(cpu_id[3]);
        caps.sse2 = Common::Bit<26>(cpu_id[3]);
        caps.sse3 = Common::Bit<0>(cpu_id[2]);
        caps.ssse3 = Common::Bit<9>(cpu_id[2]);
        caps.sse4_1 = Common::Bit<19>(cpu_id[2]);
        caps.sse4_2 = Common::Bit<20>(cpu_id[2]);
        caps.aes = Common::Bit<25>(cpu_id[2]);

        // AVX support requires 3 separate checks:
        //  - Is the AVX bit set in CPUID?
        //  - Is the XSAVE bit set in CPUID?
        //  - XGETBV result has the XCR bit set.
        if (Common::Bit<28>(cpu_id[2]) && Common::Bit<27>(cpu_id[2])) {
            if ((_xgetbv(_XCR_XFEATURE_ENABLED_MASK) & 0x6) == 0x6) {
                caps.avx = true;
                if (Common::Bit<12>(cpu_id[2]))
                    caps.fma = true;
            }
        }

        if (max_std_fn >= 7) {
            __cpuidex(cpu_id, 0x00000007, 0x00000000);
            // Can't enable AVX2 unless the XSAVE/XGETBV checks above passed
            caps.avx2 = caps.avx && Common::Bit<5>(cpu_id[1]);
            caps.bmi1 = Common::Bit<3>(cpu_id[1]);
            caps.bmi2 = Common::Bit<8>(cpu_id[1]);
            // Checks for AVX512F, AVX512CD, AVX512VL, AVX512DQ, AVX512BW (Intel Skylake-X/SP)
            if (Common::Bit<16>(cpu_id[1]) && Common::Bit<28>(cpu_id[1]) &&
                Common::Bit<31>(cpu_id[1]) && Common::Bit<17>(cpu_id[1]) &&
                Common::Bit<30>(cpu_id[1])) {
                caps.avx512 = caps.avx2;
            }
        }
    }

    if (max_ex_fn >= 0x80000004) {
        // Extract CPU model string
        __cpuid(cpu_id, 0x80000002);
        std::memcpy(caps.cpu_string, cpu_id, sizeof(cpu_id));
        __cpuid(cpu_id, 0x80000003);
        std::memcpy(caps.cpu_string + 16, cpu_id, sizeof(cpu_id));
        __cpuid(cpu_id, 0x80000004);
        std::memcpy(caps.cpu_string + 32, cpu_id, sizeof(cpu_id));
    }

    if (max_ex_fn >= 0x80000001) {
        // Check for more features
        __cpuid(cpu_id, 0x80000001);
        caps.lzcnt = Common::Bit<5>(cpu_id[2]);
        caps.fma4 = Common::Bit<16>(cpu_id[2]);
    }

    if (max_ex_fn >= 0x80000007) {
        __cpuid(cpu_id, 0x80000007);
        caps.invariant_tsc = Common::Bit<8>(cpu_id[3]);
    }

    if (max_std_fn >= 0x16) {
        __cpuid(cpu_id, 0x16);
        caps.base_frequency = cpu_id[0];
        caps.max_frequency = cpu_id[1];
        caps.bus_frequency = cpu_id[2];
    }

    return caps;
}

const CPUCaps& GetCPUCaps() {
    static CPUCaps caps = Detect();
    return caps;
}

} // namespace Common
