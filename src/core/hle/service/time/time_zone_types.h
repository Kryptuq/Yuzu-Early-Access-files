// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"

namespace Service::Time::TimeZone {

using LocationName = std::array<char, 0x24>;

/// https://switchbrew.org/wiki/Glue_services#ttinfo
struct TimeTypeInfo {
    s32 gmt_offset{};
    u8 is_dst{};
    INSERT_PADDING_BYTES(3);
    s32 abbreviation_list_index{};
    u8 is_standard_time_daylight{};
    u8 is_gmt{};
    INSERT_PADDING_BYTES(2);
};
static_assert(sizeof(TimeTypeInfo) == 0x10, "TimeTypeInfo is incorrect size");

/// https://switchbrew.org/wiki/Glue_services#TimeZoneRule
struct TimeZoneRule {
    s32 time_count{};
    s32 type_count{};
    s32 char_count{};
    u8 go_back{};
    u8 go_ahead{};
    INSERT_PADDING_BYTES(2);
    std::array<s64, 1000> ats{};
    std::array<s8, 1000> types{};
    std::array<TimeTypeInfo, 128> ttis{};
    std::array<char, 512> chars{};
    s32 default_type{};
    INSERT_PADDING_BYTES(0x12C4);
};
static_assert(sizeof(TimeZoneRule) == 0x4000, "TimeZoneRule is incorrect size");

/// https://switchbrew.org/wiki/Glue_services#CalendarAdditionalInfo
struct CalendarAdditionalInfo {
    u32 day_of_week;
    u32 day_of_year;
    std::array<char, 8> timezone_name;
    u32 is_dst;
    s32 gmt_offset;
};
static_assert(sizeof(CalendarAdditionalInfo) == 0x18, "CalendarAdditionalInfo is incorrect size");

/// https://switchbrew.org/wiki/Glue_services#CalendarTime
struct CalendarTime {
    s16 year;
    s8 month;
    s8 day;
    s8 hour;
    s8 minute;
    s8 second;
    INSERT_PADDING_BYTES_NOINIT(1);
};
static_assert(sizeof(CalendarTime) == 0x8, "CalendarTime is incorrect size");

struct CalendarInfo {
    CalendarTime time;
    CalendarAdditionalInfo additional_info;
};
static_assert(sizeof(CalendarInfo) == 0x20, "CalendarInfo is incorrect size");

struct TzifHeader {
    u32_be magic{};
    u8 version{};
    INSERT_PADDING_BYTES(15);
    s32_be ttis_gmt_count{};
    s32_be ttis_std_count{};
    s32_be leap_count{};
    s32_be time_count{};
    s32_be type_count{};
    s32_be char_count{};
};
static_assert(sizeof(TzifHeader) == 0x2C, "TzifHeader is incorrect size");

} // namespace Service::Time::TimeZone
