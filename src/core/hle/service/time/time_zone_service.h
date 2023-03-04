// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    void GetDeviceLocationName(HLERequestContext& ctx);
    void LoadTimeZoneRule(HLERequestContext& ctx);
    void ToCalendarTime(HLERequestContext& ctx);
    void ToCalendarTimeWithMyRule(HLERequestContext& ctx);
    void ToPosixTime(HLERequestContext& ctx);
    void ToPosixTimeWithMyRule(HLERequestContext& ctx);

private:
    TimeZone::TimeZoneContentManager& time_zone_content_manager;
};

} // namespace Service::Time
