// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/steady_clock.h"
#include "common/uint128.h"
#include "common/wall_clock.h"

#ifdef ARCHITECTURE_x86_64
#include "common/x64/cpu_detect.h"
#include "common/x64/native_clock.h"
#endif

namespace Common {

class StandardWallClock final : public WallClock {
public:
    explicit StandardWallClock(u64 emulated_cpu_frequency_, u64 emulated_clock_frequency_)
        : WallClock{emulated_cpu_frequency_, emulated_clock_frequency_, false} {
        start_time = SteadyClock::Now();
    }

    std::chrono::nanoseconds GetTimeNS() override {
        return SteadyClock::Now() - start_time;
    }

    std::chrono::microseconds GetTimeUS() override {
        return std::chrono::duration_cast<std::chrono::microseconds>(GetTimeNS());
    }

    std::chrono::milliseconds GetTimeMS() override {
        return std::chrono::duration_cast<std::chrono::milliseconds>(GetTimeNS());
    }

    u64 GetClockCycles() override {
        const u128 temp = Common::Multiply64Into128(GetTimeNS().count(), emulated_clock_frequency);
        return Common::Divide128On32(temp, NS_RATIO).first;
    }

    u64 GetCPUCycles() override {
        const u128 temp = Common::Multiply64Into128(GetTimeNS().count(), emulated_cpu_frequency);
        return Common::Divide128On32(temp, NS_RATIO).first;
    }

    void Pause([[maybe_unused]] bool is_paused) override {
        // Do nothing in this clock type.
    }

private:
    SteadyClock::time_point start_time;
};

#ifdef ARCHITECTURE_x86_64

std::unique_ptr<WallClock> CreateBestMatchingClock(u64 emulated_cpu_frequency,
                                                   u64 emulated_clock_frequency) {
    const auto& caps = GetCPUCaps();
    u64 rtsc_frequency = 0;
    if (caps.invariant_tsc) {
        rtsc_frequency = caps.tsc_frequency ? caps.tsc_frequency : EstimateRDTSCFrequency();
    }

    // Fallback to StandardWallClock if the hardware TSC does not have the precision greater than:
    // - A nanosecond
    // - The emulated CPU frequency
    // - The emulated clock counter frequency (CNTFRQ)
    if (rtsc_frequency <= WallClock::NS_RATIO || rtsc_frequency <= emulated_cpu_frequency ||
        rtsc_frequency <= emulated_clock_frequency) {
        return std::make_unique<StandardWallClock>(emulated_cpu_frequency,
                                                   emulated_clock_frequency);
    } else {
        return std::make_unique<X64::NativeClock>(emulated_cpu_frequency, emulated_clock_frequency,
                                                  rtsc_frequency);
    }
}

#else

std::unique_ptr<WallClock> CreateBestMatchingClock(u64 emulated_cpu_frequency,
                                                   u64 emulated_clock_frequency) {
    return std::make_unique<StandardWallClock>(emulated_cpu_frequency, emulated_clock_frequency);
}

#endif

std::unique_ptr<WallClock> CreateStandardWallClock(u64 emulated_cpu_frequency,
                                                   u64 emulated_clock_frequency) {
    return std::make_unique<StandardWallClock>(emulated_cpu_frequency, emulated_clock_frequency);
}

} // namespace Common
