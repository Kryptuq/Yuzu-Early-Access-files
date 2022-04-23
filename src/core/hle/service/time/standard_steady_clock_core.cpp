// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hardware_properties.h"
#include "core/hle/service/time/standard_steady_clock_core.h"

namespace Service::Time::Clock {

TimeSpanType StandardSteadyClockCore::GetCurrentRawTimePoint(Core::System& system) {
    const TimeSpanType ticks_time_span{
        TimeSpanType::FromTicks(system.CoreTiming().GetClockTicks(), Core::Hardware::CNTFREQ)};
    TimeSpanType raw_time_point{setup_value.nanoseconds + ticks_time_span.nanoseconds};

    if (raw_time_point.nanoseconds < cached_raw_time_point.nanoseconds) {
        raw_time_point.nanoseconds = cached_raw_time_point.nanoseconds;
    }

    cached_raw_time_point = raw_time_point;
    return raw_time_point;
}

} // namespace Service::Time::Clock
