// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"
#include "core/hle/service/time/clock_types.h"
#include "core/hle/service/time/time_zone_types.h"

namespace Service::Time::TimeZone {

class TimeZoneManager final {
public:
    TimeZoneManager();
    ~TimeZoneManager();

    void SetTotalLocationNameCount(std::size_t value) {
        total_location_name_count = value;
    }

    void SetTimeZoneRuleVersion(const u128& value) {
        time_zone_rule_version = value;
    }

    void MarkAsInitialized() {
        is_initialized = true;
    }

    Result SetDeviceLocationNameWithTimeZoneRule(const std::string& location_name,
                                                 FileSys::VirtualFile& vfs_file);
    Result SetUpdatedTime(const Clock::SteadyClockTimePoint& value);
    Result GetDeviceLocationName(TimeZone::LocationName& value) const;
    Result ToCalendarTime(const TimeZoneRule& rules, s64 time, CalendarInfo& calendar) const;
    Result ToCalendarTimeWithMyRules(s64 time, CalendarInfo& calendar) const;
    Result ParseTimeZoneRuleBinary(TimeZoneRule& rules, FileSys::VirtualFile& vfs_file) const;
    Result ToPosixTime(const TimeZoneRule& rules, const CalendarTime& calendar_time,
                       s64& posix_time) const;
    Result ToPosixTimeWithMyRule(const CalendarTime& calendar_time, s64& posix_time) const;

private:
    bool is_initialized{};
    TimeZoneRule time_zone_rule{};
    std::string device_location_name{"GMT"};
    u128 time_zone_rule_version{};
    std::size_t total_location_name_count{};
    Clock::SteadyClockTimePoint time_zone_update_time_point{
        Clock::SteadyClockTimePoint::GetRandom()};
};

} // namespace Service::Time::TimeZone
