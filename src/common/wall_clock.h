// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <chrono>
#include <memory>

#include "common/common_types.h"

namespace Common {

class WallClock {
public:
    virtual ~WallClock() = default;

    /// Returns current wall time in nanoseconds
    [[nodiscard]] virtual std::chrono::nanoseconds GetTimeNS() = 0;

    /// Returns current wall time in microseconds
    [[nodiscard]] virtual std::chrono::microseconds GetTimeUS() = 0;

    /// Returns current wall time in milliseconds
    [[nodiscard]] virtual std::chrono::milliseconds GetTimeMS() = 0;

    /// Returns current wall time in emulated clock cycles
    [[nodiscard]] virtual u64 GetClockCycles() = 0;

    /// Returns current wall time in emulated cpu cycles
    [[nodiscard]] virtual u64 GetCPUCycles() = 0;

    virtual void Pause(bool is_paused) = 0;

    /// Tells if the wall clock, uses the host CPU's hardware clock
    [[nodiscard]] bool IsNative() const {
        return is_native;
    }

protected:
    explicit WallClock(u64 emulated_cpu_frequency_, u64 emulated_clock_frequency_, bool is_native_)
        : emulated_cpu_frequency{emulated_cpu_frequency_},
          emulated_clock_frequency{emulated_clock_frequency_}, is_native{is_native_} {}

    u64 emulated_cpu_frequency;
    u64 emulated_clock_frequency;

private:
    bool is_native;
};

[[nodiscard]] std::unique_ptr<WallClock> CreateBestMatchingClock(u32 emulated_cpu_frequency,
                                                                 u32 emulated_clock_frequency);

} // namespace Common
