// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/time/clock_types.h"
#include "core/hle/service/time/steady_clock_core.h"

namespace Core {
class System;
}

namespace Service::Time::Clock {

class TickBasedSteadyClockCore final : public SteadyClockCore {
public:
    TimeSpanType GetInternalOffset() const override {
        return {};
    }

    void SetInternalOffset(TimeSpanType internal_offset) override {}

    SteadyClockTimePoint GetTimePoint(Core::System& system) override;

    TimeSpanType GetCurrentRawTimePoint(Core::System& system) override;
};

} // namespace Service::Time::Clock
