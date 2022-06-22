// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <tuple>

#include "common/bit_util.h"
#include "common/common_types.h"

namespace Core {

namespace Hardware {

// The below clock rate is based on Switch's clockspeed being widely known as 1.020GHz
// The exact value used is of course unverified.
constexpr u64 BASE_CLOCK_RATE = 1019215872; // Switch cpu frequency is 1020MHz un/docked
constexpr u64 CNTFREQ = 19200000;           // Switch's hardware clock speed
constexpr u32 NUM_CPU_CORES = 4;            // Number of CPU Cores

// Virtual to Physical core map.
constexpr std::array<s32, Common::BitSize<u64>()> VirtualToPhysicalCoreMap{
    0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
};

// Cortex-A57 supports 4 memory watchpoints
constexpr u64 NUM_WATCHPOINTS = 4;

} // namespace Hardware

} // namespace Core
