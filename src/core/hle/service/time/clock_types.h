// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/uuid.h"
#include "core/hle/service/time/errors.h"
#include "core/hle/service/time/time_zone_types.h"

namespace Service::Time::Clock {

enum class TimeType : u8 {
    UserSystemClock,
    NetworkSystemClock,
    LocalSystemClock,
};

/// https://switchbrew.org/wiki/Glue_services#SteadyClockTimePoint
struct SteadyClockTimePoint {
    s64 time_point;
    Common::UUID clock_source_id;

    ResultCode GetSpanBetween(SteadyClockTimePoint other, s64& span) const {
        span = 0;

        if (clock_source_id != other.clock_source_id) {
            return ERROR_TIME_MISMATCH;
        }

        span = other.time_point - time_point;

        return ResultSuccess;
    }

    static SteadyClockTimePoint GetRandom() {
        return {0, Common::UUID::Generate()};
    }
};
static_assert(sizeof(SteadyClockTimePoint) == 0x18, "SteadyClockTimePoint is incorrect size");
static_assert(std::is_trivially_copyable_v<SteadyClockTimePoint>,
              "SteadyClockTimePoint must be trivially copyable");

struct SteadyClockContext {
    u64 internal_offset;
    Common::UUID steady_time_point;
};
static_assert(sizeof(SteadyClockContext) == 0x18, "SteadyClockContext is incorrect size");
static_assert(std::is_trivially_copyable_v<SteadyClockContext>,
              "SteadyClockContext must be trivially copyable");

struct SystemClockContext {
    s64 offset;
    SteadyClockTimePoint steady_time_point;
};
static_assert(sizeof(SystemClockContext) == 0x20, "SystemClockContext is incorrect size");
static_assert(std::is_trivially_copyable_v<SystemClockContext>,
              "SystemClockContext must be trivially copyable");

/// https://switchbrew.org/wiki/Glue_services#TimeSpanType
struct TimeSpanType {
    s64 nanoseconds{};
    static constexpr s64 ns_per_second{1000000000ULL};

    s64 ToSeconds() const {
        return nanoseconds / ns_per_second;
    }

    static TimeSpanType FromSeconds(s64 seconds) {
        return {seconds * ns_per_second};
    }

    static TimeSpanType FromTicks(u64 ticks, u64 frequency) {
        return FromSeconds(static_cast<s64>(ticks) / static_cast<s64>(frequency));
    }
};
static_assert(sizeof(TimeSpanType) == 8, "TimeSpanType is incorrect size");

struct ClockSnapshot {
    SystemClockContext user_context;
    SystemClockContext network_context;
    s64 user_time;
    s64 network_time;
    TimeZone::CalendarTime user_calendar_time;
    TimeZone::CalendarTime network_calendar_time;
    TimeZone::CalendarAdditionalInfo user_calendar_additional_time;
    TimeZone::CalendarAdditionalInfo network_calendar_additional_time;
    SteadyClockTimePoint steady_clock_time_point;
    TimeZone::LocationName location_name;
    u8 is_automatic_correction_enabled;
    TimeType type;
    INSERT_PADDING_BYTES_NOINIT(0x2);

    static ResultCode GetCurrentTime(s64& current_time,
                                     const SteadyClockTimePoint& steady_clock_time_point,
                                     const SystemClockContext& context) {
        if (steady_clock_time_point.clock_source_id != context.steady_time_point.clock_source_id) {
            current_time = 0;
            return ERROR_TIME_MISMATCH;
        }
        current_time = steady_clock_time_point.time_point + context.offset;
        return ResultSuccess;
    }
};
static_assert(sizeof(ClockSnapshot) == 0xD0, "ClockSnapshot is incorrect size");

} // namespace Service::Time::Clock
