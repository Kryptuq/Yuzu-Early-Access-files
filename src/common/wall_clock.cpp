// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/uint128.h"
#include "common/wall_clock.h"

#ifdef ARCHITECTURE_x86_64
#include "common/x64/cpu_detect.h"
#include "common/x64/native_clock.h"
#endif

namespace Common {

using base_timer = std::chrono::steady_clock;
using base_time_point = std::chrono::time_point<base_timer>;

class StandardWallClock final : public WallClock {
public:
    explicit StandardWallClock(u64 emulated_cpu_frequency_, u64 emulated_clock_frequency_)
        : WallClock(emulated_cpu_frequency_, emulated_clock_frequency_, false) {
        start_time = base_timer::now();
    }

    std::chrono::nanoseconds GetTimeNS() override {
        base_time_point current = base_timer::now();
        auto elapsed = current - start_time;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
    }

    std::chrono::microseconds GetTimeUS() override {
        base_time_point current = base_timer::now();
        auto elapsed = current - start_time;
        return std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    }

    std::chrono::milliseconds GetTimeMS() override {
        base_time_point current = base_timer::now();
        auto elapsed = current - start_time;
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    }

    u64 GetClockCycles() override {
        std::chrono::nanoseconds time_now = GetTimeNS();
        const u128 temporary =
            Common::Multiply64Into128(time_now.count(), emulated_clock_frequency);
        return Common::Divide128On32(temporary, 1000000000).first;
    }

    u64 GetCPUCycles() override {
        std::chrono::nanoseconds time_now = GetTimeNS();
        const u128 temporary = Common::Multiply64Into128(time_now.count(), emulated_cpu_frequency);
        return Common::Divide128On32(temporary, 1000000000).first;
    }

    void Pause([[maybe_unused]] bool is_paused) override {
        // Do nothing in this clock type.
    }

private:
    base_time_point start_time;
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

} // namespace Common
