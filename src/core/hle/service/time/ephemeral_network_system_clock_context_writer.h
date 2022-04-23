// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/time/system_clock_context_update_callback.h"

namespace Service::Time::Clock {

class EphemeralNetworkSystemClockContextWriter final : public SystemClockContextUpdateCallback {
public:
    EphemeralNetworkSystemClockContextWriter() : SystemClockContextUpdateCallback{} {}
};

} // namespace Service::Time::Clock
