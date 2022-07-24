// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/kernel_helpers.h"
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

    ~StandardUserSystemClockCore() override;

    Result SetAutomaticCorrectionEnabled(Core::System& system, bool value);

    Result GetClockContext(Core::System& system, SystemClockContext& ctx) const override;

    bool IsAutomaticCorrectionEnabled() const {
        return auto_correction_enabled;
    }

    void SetAutomaticCorrectionUpdatedTime(SteadyClockTimePoint steady_clock_time_point) {
        auto_correction_time = steady_clock_time_point;
    }

protected:
    Result Flush(const SystemClockContext&) override;

    Result SetClockContext(const SystemClockContext&) override;

    Result ApplyAutomaticCorrection(Core::System& system, bool value) const;

    const SteadyClockTimePoint& GetAutomaticCorrectionUpdatedTime() const {
        return auto_correction_time;
    }

private:
    StandardLocalSystemClockCore& local_system_clock_core;
    StandardNetworkSystemClockCore& network_system_clock_core;
    bool auto_correction_enabled{};
    SteadyClockTimePoint auto_correction_time;
    KernelHelpers::ServiceContext service_context;
    Kernel::KEvent* auto_correction_event;
};

} // namespace Service::Time::Clock
