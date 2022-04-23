// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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

    union alignas(16) TimePoint {
        TimePoint() : pack{} {}
        u128 pack{};
        struct Inner {
            u64 last_measure{};
            u64 accumulated_ticks{};
        } inner;
    };

    /// value used to reduce the native clocks accuracy as some apss rely on
    /// undefined behavior where the level of accuracy in the clock shouldn't
    /// be higher.
    static constexpr u64 inaccuracy_mask = ~(UINT64_C(0x400) - 1);

    TimePoint time_point;
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
