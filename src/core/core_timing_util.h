// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <chrono>

#include "common/common_types.h"
#include "core/hardware_properties.h"

namespace Core::Timing {

namespace detail {
constexpr u64 CNTFREQ_ADJUSTED = Hardware::CNTFREQ / 1000;
constexpr u64 BASE_CLOCK_RATE_ADJUSTED = Hardware::BASE_CLOCK_RATE / 1000;
} // namespace detail

[[nodiscard]] constexpr s64 msToCycles(std::chrono::milliseconds ms) {
    return ms.count() * detail::BASE_CLOCK_RATE_ADJUSTED;
}

[[nodiscard]] constexpr s64 usToCycles(std::chrono::microseconds us) {
    return us.count() * detail::BASE_CLOCK_RATE_ADJUSTED / 1000;
}

[[nodiscard]] constexpr s64 nsToCycles(std::chrono::nanoseconds ns) {
    return ns.count() * detail::BASE_CLOCK_RATE_ADJUSTED / 1000000;
}

[[nodiscard]] constexpr u64 msToClockCycles(std::chrono::milliseconds ms) {
    return static_cast<u64>(ms.count()) * detail::CNTFREQ_ADJUSTED;
}

[[nodiscard]] constexpr u64 usToClockCycles(std::chrono::microseconds us) {
    return us.count() * detail::CNTFREQ_ADJUSTED / 1000;
}

[[nodiscard]] constexpr u64 nsToClockCycles(std::chrono::nanoseconds ns) {
    return ns.count() * detail::CNTFREQ_ADJUSTED / 1000000;
}

[[nodiscard]] constexpr u64 CpuCyclesToClockCycles(u64 ticks) {
    return ticks * detail::CNTFREQ_ADJUSTED / detail::BASE_CLOCK_RATE_ADJUSTED;
}

[[nodiscard]] constexpr std::chrono::milliseconds CyclesToMs(s64 cycles) {
    return std::chrono::milliseconds(cycles / detail::BASE_CLOCK_RATE_ADJUSTED);
}

[[nodiscard]] constexpr std::chrono::nanoseconds CyclesToNs(s64 cycles) {
    return std::chrono::nanoseconds(cycles * 1000000 / detail::BASE_CLOCK_RATE_ADJUSTED);
}

[[nodiscard]] constexpr std::chrono::microseconds CyclesToUs(s64 cycles) {
    return std::chrono::microseconds(cycles * 1000 / detail::BASE_CLOCK_RATE_ADJUSTED);
}

} // namespace Core::Timing
