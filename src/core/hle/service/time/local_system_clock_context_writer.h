// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/time/system_clock_context_update_callback.h"
#include "core/hle/service/time/time_sharedmemory.h"

namespace Service::Time::Clock {

class LocalSystemClockContextWriter final : public SystemClockContextUpdateCallback {
public:
    explicit LocalSystemClockContextWriter(SharedMemory& shared_memory_)
        : SystemClockContextUpdateCallback{}, shared_memory{shared_memory_} {}

protected:
    ResultCode Update() override {
        shared_memory.UpdateLocalSystemClockContext(context);
        return ResultSuccess;
    }

private:
    SharedMemory& shared_memory;
};

} // namespace Service::Time::Clock
