// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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

        void GetStandardUserSystemClock(Kernel::HLERequestContext& ctx);
        void GetStandardNetworkSystemClock(Kernel::HLERequestContext& ctx);
        void GetStandardSteadyClock(Kernel::HLERequestContext& ctx);
        void GetTimeZoneService(Kernel::HLERequestContext& ctx);
        void GetStandardLocalSystemClock(Kernel::HLERequestContext& ctx);
        void IsStandardNetworkSystemClockAccuracySufficient(Kernel::HLERequestContext& ctx);
        void CalculateMonotonicSystemClockBaseTimePoint(Kernel::HLERequestContext& ctx);
        void GetClockSnapshot(Kernel::HLERequestContext& ctx);
        void GetClockSnapshotFromSystemClockContext(Kernel::HLERequestContext& ctx);
        void CalculateStandardUserSystemClockDifferenceByUser(Kernel::HLERequestContext& ctx);
        void CalculateSpanBetween(Kernel::HLERequestContext& ctx);
        void GetSharedMemoryNativeHandle(Kernel::HLERequestContext& ctx);

    private:
        ResultCode GetClockSnapshotFromSystemClockContextInternal(
            Kernel::KThread* thread, Clock::SystemClockContext user_context,
            Clock::SystemClockContext network_context, Clock::TimeType type,
            Clock::ClockSnapshot& cloc_snapshot);

    protected:
        std::shared_ptr<Module> module;
    };
};

/// Registers all Time services with the specified service manager.
void InstallInterfaces(Core::System& system);

} // namespace Service::Time
