// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/time/system_clock_core.h"

namespace Service::Time::Clock {

class EphemeralNetworkSystemClockCore final : public SystemClockCore {
public:
    explicit EphemeralNetworkSystemClockCore(SteadyClockCore& steady_clock_core_)
        : SystemClockCore{steady_clock_core_} {}
};

} // namespace Service::Time::Clock
