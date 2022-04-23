// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include "common/common_types.h"

namespace Core::Timing {
class CoreTiming;
}

namespace Service::APM {

enum class PerformanceConfiguration : u32 {
    Config1 = 0x00010000,
    Config2 = 0x00010001,
    Config3 = 0x00010002,
    Config4 = 0x00020000,
    Config5 = 0x00020001,
    Config6 = 0x00020002,
    Config7 = 0x00020003,
    Config8 = 0x00020004,
    Config9 = 0x00020005,
    Config10 = 0x00020006,
    Config11 = 0x92220007,
    Config12 = 0x92220008,
    Config13 = 0x92220009,
    Config14 = 0x9222000A,
    Config15 = 0x9222000B,
    Config16 = 0x9222000C,
};

// This is nn::oe::CpuBoostMode
enum class CpuBoostMode : u32 {
    Normal = 0,   // Boost mode disabled
    FastLoad = 1, // CPU + GPU -> Config 13, 14, 15, or 16
    Partial = 2,  // GPU Only -> Config 15 or 16
};

// This is nn::oe::PerformanceMode
enum class PerformanceMode : s32 {
    Invalid = -1,
    Normal = 0,
    Boost = 1,
};

// Class to manage the state and change of the emulated system performance.
// Specifically, this deals with PerformanceMode, which corresponds to the system being docked or
// undocked, and PerformanceConfig which specifies the exact CPU, GPU, and Memory clocks to operate
// at. Additionally, this manages 'Boost Mode', which allows games to temporarily overclock the
// system during times of high load -- this simply maps to different PerformanceConfigs to use.
class Controller {
public:
    explicit Controller(Core::Timing::CoreTiming& core_timing_);
    ~Controller();

    void SetPerformanceConfiguration(PerformanceMode mode, PerformanceConfiguration config);
    void SetFromCpuBoostMode(CpuBoostMode mode);

    PerformanceMode GetCurrentPerformanceMode() const;
    PerformanceConfiguration GetCurrentPerformanceConfiguration(PerformanceMode mode);

private:
    void SetClockSpeed(u32 mhz);

    [[maybe_unused]] Core::Timing::CoreTiming& core_timing;

    std::map<PerformanceMode, PerformanceConfiguration> configs;
};

} // namespace Service::APM
