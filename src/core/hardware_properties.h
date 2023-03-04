// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <tuple>

#include "common/bit_util.h"
#include "common/common_types.h"

namespace Core {

namespace Hardware {

constexpr u64 BASE_CLOCK_RATE = 1'020'000'000; // Default CPU Frequency = 1020 MHz
constexpr u64 CNTFREQ = 19'200'000;            // CNTPCT_EL0 Frequency = 19.2 MHz
constexpr u32 NUM_CPU_CORES = 4;               // Number of CPU Cores

// Virtual to Physical core map.
constexpr std::array<s32, Common::BitSize<u64>()> VirtualToPhysicalCoreMap{
    0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
};

static constexpr inline size_t NumVirtualCores = Common::BitSize<u64>();

static constexpr inline u64 VirtualCoreMask = [] {
    u64 mask = 0;
    for (size_t i = 0; i < NumVirtualCores; ++i) {
        mask |= (UINT64_C(1) << i);
    }
    return mask;
}();

static constexpr inline u64 ConvertVirtualCoreMaskToPhysical(u64 v_core_mask) {
    u64 p_core_mask = 0;
    while (v_core_mask != 0) {
        const u64 next = std::countr_zero(v_core_mask);
        v_core_mask &= ~(static_cast<u64>(1) << next);
        p_core_mask |= (static_cast<u64>(1) << VirtualToPhysicalCoreMap[next]);
    }
    return p_core_mask;
}

// Cortex-A57 supports 4 memory watchpoints
constexpr u64 NUM_WATCHPOINTS = 4;

} // namespace Hardware

} // namespace Core
