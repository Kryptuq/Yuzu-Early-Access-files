// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/time/system_clock_core.h"

namespace Service::Time::Clock {

class EphemeralNetworkSystemClockCore final : public SystemClockCore {
public:
    explicit EphemeralNetworkSystemClockCore(SteadyClockCore& steady_clock_core_)
        : SystemClockCore{steady_clock_core_} {}
};

} // namespace Service::Time::Clock
