// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/time/clock_types.h"
#include "core/hle/service/time/steady_clock_core.h"

namespace Core {
class System;
}

namespace Service::Time::Clock {

class StandardSteadyClockCore final : public SteadyClockCore {
public:
    SteadyClockTimePoint GetTimePoint(Core::System& system) override {
        return {GetCurrentRawTimePoint(system).ToSeconds(), GetClockSourceId()};
    }

    TimeSpanType GetInternalOffset() const override {
        return internal_offset;
    }

    void SetInternalOffset(TimeSpanType value) override {
        internal_offset = value;
    }

    TimeSpanType GetCurrentRawTimePoint(Core::System& system) override;

    void SetSetupValue(TimeSpanType value) {
        setup_value = value;
    }

private:
    TimeSpanType setup_value{};
    TimeSpanType internal_offset{};
    TimeSpanType cached_raw_time_point{};
};

} // namespace Service::Time::Clock
