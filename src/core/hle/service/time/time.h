// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"
#include "core/hle/service/time/clock_types.h"

namespace Core {
class System;
}

namespace Service::Time {

class Module final {
public:
    Module() = default;

    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(std::shared_ptr<Module> module_, Core::System& system_,
                           const char* name);
        ~Interface() override;

        void GetStandardUserSystemClock(HLERequestContext& ctx);
        void GetStandardNetworkSystemClock(HLERequestContext& ctx);
        void GetStandardSteadyClock(HLERequestContext& ctx);
        void GetTimeZoneService(HLERequestContext& ctx);
        void GetStandardLocalSystemClock(HLERequestContext& ctx);
        void IsStandardNetworkSystemClockAccuracySufficient(HLERequestContext& ctx);
        void CalculateMonotonicSystemClockBaseTimePoint(HLERequestContext& ctx);
        void GetClockSnapshot(HLERequestContext& ctx);
        void GetClockSnapshotFromSystemClockContext(HLERequestContext& ctx);
        void CalculateStandardUserSystemClockDifferenceByUser(HLERequestContext& ctx);
        void CalculateSpanBetween(HLERequestContext& ctx);
        void GetSharedMemoryNativeHandle(HLERequestContext& ctx);

    private:
        Result GetClockSnapshotFromSystemClockContextInternal(
            Kernel::KThread* thread, Clock::SystemClockContext user_context,
            Clock::SystemClockContext network_context, Clock::TimeType type,
            Clock::ClockSnapshot& cloc_snapshot);

    protected:
        std::shared_ptr<Module> module;
    };
};

void LoopProcess(Core::System& system);

} // namespace Service::Time
