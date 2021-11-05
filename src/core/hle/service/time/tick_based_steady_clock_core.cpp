// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hardware_properties.h"
#include "core/hle/service/time/tick_based_steady_clock_core.h"

namespace Service::Time::Clock {

SteadyClockTimePoint TickBasedSteadyClockCore::GetTimePoint(Core::System& system) {
    const TimeSpanType ticks_time_span{
        TimeSpanType::FromTicks(system.CoreTiming().GetClockTicks(), Core::Hardware::CNTFREQ)};

    return {ticks_time_span.ToSeconds(), GetClockSourceId()};
}

TimeSpanType TickBasedSteadyClockCore::GetCurrentRawTimePoint(Core::System& system) {
    return TimeSpanType::FromSeconds(GetTimePoint(system).time_point);
}

} // namespace Service::Time::Clock
