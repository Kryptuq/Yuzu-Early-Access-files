// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Time {

namespace TimeZone {
class TimeZoneContentManager;
}

class ITimeZoneService final : public ServiceFramework<ITimeZoneService> {
public:
    explicit ITimeZoneService(Core::System& system_,
                              TimeZone::TimeZoneContentManager& time_zone_manager_);

private:
    void GetDeviceLocationName(Kernel::HLERequestContext& ctx);
    void LoadTimeZoneRule(Kernel::HLERequestContext& ctx);
    void ToCalendarTime(Kernel::HLERequestContext& ctx);
    void ToCalendarTimeWithMyRule(Kernel::HLERequestContext& ctx);
    void ToPosixTime(Kernel::HLERequestContext& ctx);
    void ToPosixTimeWithMyRule(Kernel::HLERequestContext& ctx);

private:
    TimeZone::TimeZoneContentManager& time_zone_content_manager;
};

} // namespace Service::Time
