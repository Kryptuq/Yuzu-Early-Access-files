// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/k_event.h"
#include "core/hle/service/time/clock_types.h"
#include "core/hle/service/time/system_clock_core.h"

namespace Core {
class System;
}

namespace Kernel {
class KEvent;
}

namespace Service::Time::Clock {

class StandardLocalSystemClockCore;
class StandardNetworkSystemClockCore;

class StandardUserSystemClockCore final : public SystemClockCore {
public:
    StandardUserSystemClockCore(StandardLocalSystemClockCore& local_system_clock_core_,
                                StandardNetworkSystemClockCore& network_system_clock_core_,
                                Core::System& system_);

    ResultCode SetAutomaticCorrectionEnabled(Core::System& system, bool value);

    ResultCode GetClockContext(Core::System& system, SystemClockContext& ctx) const override;

    bool IsAutomaticCorrectionEnabled() const {
        return auto_correction_enabled;
    }

    void SetAutomaticCorrectionUpdatedTime(SteadyClockTimePoint steady_clock_time_point) {
        auto_correction_time = steady_clock_time_point;
    }

protected:
    ResultCode Flush(const SystemClockContext&) override;

    ResultCode SetClockContext(const SystemClockContext&) override;

    ResultCode ApplyAutomaticCorrection(Core::System& system, bool value) const;

    const SteadyClockTimePoint& GetAutomaticCorrectionUpdatedTime() const {
        return auto_correction_time;
    }

private:
    StandardLocalSystemClockCore& local_system_clock_core;
    StandardNetworkSystemClockCore& network_system_clock_core;
    bool auto_correction_enabled{};
    SteadyClockTimePoint auto_correction_time;
    Kernel::KEvent auto_correction_event;
};

} // namespace Service::Time::Clock
