// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "common/x64/cpu_detect.h"
#include "common/x64/cpu_wait.h"

namespace Common::X64 {

#ifdef _MSC_VER
__forceinline static u64 FencedRDTSC() {
    _mm_lfence();
    _ReadWriteBarrier();
    const u64 result = __rdtsc();
    _mm_lfence();
    _ReadWriteBarrier();
    return result;
}

__forceinline static void TPAUSE() {
    // 100,000 cycles is a reasonable amount of time to wait to save on CPU resources.
    // For reference:
    // At 1 GHz, 100K cycles is 100us
    // At 2 GHz, 100K cycles is 50us
    // At 4 GHz, 100K cycles is 25us
    static constexpr auto PauseCycles = 100'000;
    _tpause(0, FencedRDTSC() + PauseCycles);
}
#else
static u64 FencedRDTSC() {
    u64 eax;
    u64 edx;
    asm volatile("lfence\n\t"
                 "rdtsc\n\t"
                 "lfence\n\t"
                 : "=a"(eax), "=d"(edx));
    return (edx << 32) | eax;
}

static void TPAUSE() {
    // 100,000 cycles is a reasonable amount of time to wait to save on CPU resources.
    // For reference:
    // At 1 GHz, 100K cycles is 100us
    // At 2 GHz, 100K cycles is 50us
    // At 4 GHz, 100K cycles is 25us
    static constexpr auto PauseCycles = 100'000;
    const auto tsc = FencedRDTSC() + PauseCycles;
    const auto eax = static_cast<u32>(tsc & 0xFFFFFFFF);
    const auto edx = static_cast<u32>(tsc >> 32);
    asm volatile("tpause %0" : : "r"(0), "d"(edx), "a"(eax));
}
#endif

void MicroSleep() {
    static const bool has_waitpkg = GetCPUCaps().waitpkg;

    if (has_waitpkg) {
        TPAUSE();
    } else {
        std::this_thread::yield();
    }
}

} // namespace Common::X64
