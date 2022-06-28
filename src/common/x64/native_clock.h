// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include "common/wall_clock.h"

namespace Common {

namespace X64 {
class NativeClock final : public WallClock {
public:
    explicit NativeClock(u64 emulated_cpu_frequency_, u64 emulated_clock_frequency_,
                         u64 rtsc_frequency_);

    std::chrono::nanoseconds GetTimeNS() override;

    std::chrono::microseconds GetTimeUS() override;

    std::chrono::milliseconds GetTimeMS() override;

    u64 GetClockCycles() override;

    u64 GetCPUCycles() override;

    void Pause(bool is_paused) override;

private:
    u64 GetRTSC();

    struct alignas(16) TimePoint {
        u64 last_measure{};
        u64 accumulated_ticks{};
    };

    std::atomic<TimePoint> time_point;
    // factors
    u64 clock_rtsc_factor{};
    u64 cpu_rtsc_factor{};
    u64 ns_rtsc_factor{};
    u64 us_rtsc_factor{};
    u64 ms_rtsc_factor{};

    u64 rtsc_frequency;
};
} // namespace X64

u64 EstimateRDTSCFrequency();

} // namespace Common
