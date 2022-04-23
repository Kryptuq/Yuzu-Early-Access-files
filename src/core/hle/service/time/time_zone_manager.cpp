// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <climits>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/registered_cache.h"
#include "core/hle/service/time/time_zone_manager.h"

namespace Service::Time::TimeZone {

static constexpr s32 epoch_year{1970};
static constexpr s32 year_base{1900};
static constexpr s32 epoch_week_day{4};
static constexpr s32 seconds_per_minute{60};
static constexpr s32 minutes_per_hour{60};
static constexpr s32 hours_per_day{24};
static constexpr s32 days_per_week{7};
static constexpr s32 days_per_normal_year{365};
static constexpr s32 days_per_leap_year{366};
static constexpr s32 months_per_year{12};
static constexpr s32 seconds_per_hour{seconds_per_minute * minutes_per_hour};
static constexpr s32 seconds_per_day{seconds_per_hour * hours_per_day};
static constexpr s32 years_per_repeat{400};
static constexpr s64 average_seconds_per_year{31556952};
static constexpr s64 seconds_per_repeat{years_per_repeat * average_seconds_per_year};

struct Rule {
    enum class Type : u32 { JulianDay, DayOfYear, MonthNthDayOfWeek };
    Type rule_type{};
    s32 day{};
    s32 week{};
    s32 month{};
    s32 transition_time{};
};

struct CalendarTimeInternal {
    s64 year{};
    s8 month{};
    s8 day{};
    s8 hour{};
    s8 minute{};
    s8 second{};
    int Compare(const CalendarTimeInternal& other) const {
        if (year != other.year) {
            if (year < other.year) {
                return -1;
            }
            return 1;
        }
        if (month != other.month) {
            return month - other.month;
        }
        if (day != other.day) {
            return day - other.day;
        }
        if (hour != other.hour) {
            return hour - other.hour;
        }
        if (minute != other.minute) {
            return minute - other.minute;
        }
        if (second != other.second) {
            return second - other.second;
        }
        return {};
    }
};

template <typename TResult, typename TOperand>
static bool SafeAdd(TResult& result, TOperand op) {
    result = result + op;
    return true;
}

template <typename TResult, typename TUnit, typename TBase>
static bool SafeNormalize(TResult& result, TUnit& unit, TBase base) {
    TUnit delta{};
    if (unit >= 0) {
        delta = unit / base;
    } else {
        delta = -1 - (-1 - unit) / base;
    }
    unit -= delta * base;
    return SafeAdd(result, delta);
}

template <typename T>
static constexpr bool IsLeapYear(T year) {
    return ((year) % 4) == 0 && (((year) % 100) != 0 || ((year) % 400) == 0);
}

template <typename T>
static constexpr T GetYearLengthInDays(T year) {
    return IsLeapYear(year) ? days_per_leap_year : days_per_normal_year;
}

static constexpr s64 GetLeapDaysFromYearPositive(s64 year) {
    return year / 4 - year / 100 + year / years_per_repeat;
}

static constexpr s64 GetLeapDaysFromYear(s64 year) {
    if (year < 0) {
        return -1 - GetLeapDaysFromYearPositive(-1 - year);
    } else {
        return GetLeapDaysFromYearPositive(year);
    }
}

static constexpr int GetMonthLength(bool is_leap_year, int month) {
    constexpr std::array<int, 12> month_lengths{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    constexpr std::array<int, 12> month_lengths_leap{31, 29, 31, 30, 31, 30,
                                                     31, 31, 30, 31, 30, 31};
    return is_leap_year ? month_lengths_leap[month] : month_lengths[month];
}

static constexpr bool IsDigit(char value) {
    return value >= '0' && value <= '9';
}

static constexpr int GetQZName(const char* name, int offset, char delimiter) {
    while (name[offset] != '\0' && name[offset] != delimiter) {
        offset++;
    }
    return offset;
}

static constexpr int GetTZName(const char* name, int offset) {
    for (char value{name[offset]};
         value != '\0' && !IsDigit(value) && value != ',' && value != '-' && value != '+';
         offset++) {
        value = name[offset];
    }
    return offset;
}

static constexpr bool GetInteger(const char* name, int& offset, int& value, int min, int max) {
    value = 0;
    char temp{name[offset]};
    if (!IsDigit(temp)) {
        return {};
    }
    do {
        value = value * 10 + (temp - '0');
        if (value > max) {
            return {};
        }
        temp = name[offset];
    } while (IsDigit(temp));

    return value >= min;
}

static constexpr bool GetSeconds(const char* name, int& offset, int& seconds) {
    seconds = 0;
    int value{};
    if (!GetInteger(name, offset, value, 0, hours_per_day * days_per_week - 1)) {
        return {};
    }
    seconds = value * seconds_per_hour;

    if (name[offset] == ':') {
        offset++;
        if (!GetInteger(name, offset, value, 0, minutes_per_hour - 1)) {
            return {};
        }
        seconds += value * seconds_per_minute;
        if (name[offset] == ':') {
            offset++;
            if (!GetInteger(name, offset, value, 0, seconds_per_minute)) {
                return {};
            }
            seconds += value;
        }
    }
    return true;
}

static constexpr bool GetOffset(const char* name, int& offset, int& value) {
    bool is_negative{};
    if (name[offset] == '-') {
        is_negative = true;
        offset++;
    } else if (name[offset] == '+') {
        offset++;
    }
    if (!GetSeconds(name, offset, value)) {
        return {};
    }
    if (is_negative) {
        value = -value;
    }
    return true;
}

static constexpr bool GetRule(const char* name, int& position, Rule& rule) {
    bool is_valid{};
    if (name[position] == 'J') {
        position++;
        rule.rule_type = Rule::Type::JulianDay;
        is_valid = GetInteger(name, position, rule.day, 1, days_per_normal_year);
    } else if (name[position] == 'M') {
        position++;
        rule.rule_type = Rule::Type::MonthNthDayOfWeek;
        is_valid = GetInteger(name, position, rule.month, 1, months_per_year);
        if (!is_valid) {
            return {};
        }
        if (name[position++] != '.') {
            return {};
        }
        is_valid = GetInteger(name, position, rule.week, 1, 5);
        if (!is_valid) {
            return {};
        }
        if (name[position++] != '.') {
            return {};
        }
        is_valid = GetInteger(name, position, rule.day, 0, days_per_week - 1);
    } else if (isdigit(name[position])) {
        rule.rule_type = Rule::Type::DayOfYear;
        is_valid = GetInteger(name, position, rule.day, 0, days_per_leap_year - 1);
    } else {
        return {};
    }
    if (!is_valid) {
        return {};
    }
    if (name[position] == '/') {
        position++;
        return GetOffset(name, position, rule.transition_time);
    } else {
        rule.transition_time = 2 * seconds_per_hour;
    }
    return true;
}

static constexpr int TransitionTime(int year, Rule rule, int offset) {
    int value{};
    switch (rule.rule_type) {
    case Rule::Type::JulianDay:
        value = (rule.day - 1) * seconds_per_day;
        if (IsLeapYear(year) && rule.day >= 60) {
            value += seconds_per_day;
        }
        break;
    case Rule::Type::DayOfYear:
        value = rule.day * seconds_per_day;
        break;
    case Rule::Type::MonthNthDayOfWeek: {
        // Use Zeller's Congruence (https://en.wikipedia.org/wiki/Zeller%27s_congruence) to
        // calculate the day of the week for any Julian or Gregorian calendar date.
        const int m1{(rule.month + 9) % 12 + 1};
        const int yy0{(rule.month <= 2) ? (year - 1) : year};
        const int yy1{yy0 / 100};
        const int yy2{yy0 % 100};
        int day_of_week{((26 * m1 - 2) / 10 + 1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7};

        if (day_of_week < 0) {
            day_of_week += days_per_week;
        }
        int day{rule.day - day_of_week};
        if (day < 0) {
            day += days_per_week;
        }
        for (int i{1}; i < rule.week; i++) {
            if (day + days_per_week >= GetMonthLength(IsLeapYear(year), rule.month - 1)) {
                break;
            }
            day += days_per_week;
        }

        value = day * seconds_per_day;
        for (int index{}; index < rule.month - 1; ++index) {
            value += GetMonthLength(IsLeapYear(year), index) * seconds_per_day;
        }
        break;
    }
    default:
        UNREACHABLE();
    }
    return value + rule.transition_time + offset;
}

static bool ParsePosixName(const char* name, TimeZoneRule& rule) {
    constexpr char default_rule[]{",M4.1.0,M10.5.0"};
    const char* std_name{name};
    int std_len{};
    int offset{};
    int std_offset{};

    if (name[offset] == '<') {
        offset++;
        std_name = name + offset;
        const int std_name_offset{offset};
        offset = GetQZName(name, offset, '>');
        if (name[offset] != '>') {
            return {};
        }
        std_len = offset - std_name_offset;
        offset++;
    } else {
        offset = GetTZName(name, offset);
        std_len = offset;
    }
    if (std_len == 0) {
        return {};
    }
    if (!GetOffset(name, offset, std_offset)) {
        return {};
    }

    int char_count{std_len + 1};
    int dest_len{};
    int dest_offset{};
    const char* dest_name{name + offset};
    if (rule.chars.size() < std::size_t(char_count)) {
        return {};
    }

    if (name[offset] != '\0') {
        if (name[offset] == '<') {
            dest_name = name + (++offset);
            const int dest_name_offset{offset};
            offset = GetQZName(name, offset, '>');
            if (name[offset] != '>') {
                return {};
            }
            dest_len = offset - dest_name_offset;
            offset++;
        } else {
            dest_name = name + (offset);
            offset = GetTZName(name, offset);
            dest_len = offset;
        }
        if (dest_len == 0) {
            return {};
        }
        char_count += dest_len + 1;
        if (rule.chars.size() < std::size_t(char_count)) {
            return {};
        }
        if (name[offset] != '\0' && name[offset] != ',' && name[offset] != ';') {
            if (!GetOffset(name, offset, dest_offset)) {
                return {};
            }
        } else {
            dest_offset = std_offset - seconds_per_hour;
        }
        if (name[offset] == '\0') {
            name = default_rule;
            offset = 0;
        }
        if (name[offset] == ',' || name[offset] == ';') {
            offset++;

            Rule start{};
            if (!GetRule(name, offset, start)) {
                return {};
            }
            if (name[offset++] != ',') {
                return {};
            }

            Rule end{};
            if (!GetRule(name, offset, end)) {
                return {};
            }
            if (name[offset] != '\0') {
                return {};
            }

            rule.type_count = 2;
            rule.ttis[0].gmt_offset = -dest_offset;
            rule.ttis[0].is_dst = true;
            rule.ttis[0].abbreviation_list_index = std_len + 1;
            rule.ttis[1].gmt_offset = -std_offset;
            rule.ttis[1].is_dst = false;
            rule.ttis[1].abbreviation_list_index = 0;
            rule.default_type = 0;

            s64 jan_first{};
            int time_count{};
            int jan_offset{};
            int year_beginning{epoch_year};
            do {
                const int year_seconds{GetYearLengthInDays(year_beginning - 1) * seconds_per_day};
                year_beginning--;
                if (!SafeAdd(jan_first, -year_seconds)) {
                    jan_offset = -year_seconds;
                    break;
                }
            } while (epoch_year - years_per_repeat / 2 < year_beginning);

            int year_limit{year_beginning + years_per_repeat + 1};
            int year{};
            for (year = year_beginning; year < year_limit; year++) {
                int start_time{TransitionTime(year, start, std_offset)};
                int end_time{TransitionTime(year, end, dest_offset)};
                const int year_seconds{GetYearLengthInDays(year) * seconds_per_day};
                const bool is_reversed{end_time < start_time};
                if (is_reversed) {
                    int swap{start_time};
                    start_time = end_time;
                    end_time = swap;
                }

                if (is_reversed ||
                    (start_time < end_time &&
                     (end_time - start_time < (year_seconds + (std_offset - dest_offset))))) {
                    if (rule.ats.size() - 2 < std::size_t(time_count)) {
                        break;
                    }

                    rule.ats[time_count] = jan_first;
                    if (SafeAdd(rule.ats[time_count], jan_offset + start_time)) {
                        rule.types[time_count++] = is_reversed ? 1 : 0;
                    } else if (jan_offset != 0) {
                        rule.default_type = is_reversed ? 1 : 0;
                    }

                    rule.ats[time_count] = jan_first;
                    if (SafeAdd(rule.ats[time_count], jan_offset + end_time)) {
                        rule.types[time_count++] = is_reversed ? 0 : 1;
                        year_limit = year + years_per_repeat + 1;
                    } else if (jan_offset != 0) {
                        rule.default_type = is_reversed ? 0 : 1;
                    }
                }
                if (!SafeAdd(jan_first, jan_offset + year_seconds)) {
                    break;
                }
                jan_offset = 0;
            }
            rule.time_count = time_count;
            if (time_count == 0) {
                rule.type_count = 1;
            } else if (years_per_repeat < year - year_beginning) {
                rule.go_back = true;
                rule.go_ahead = true;
            }
        } else {
            if (name[offset] == '\0') {
                return {};
            }

            s64 their_std_offset{};
            for (int index{}; index < rule.time_count; ++index) {
                const s8 type{rule.types[index]};
                if (rule.ttis[type].is_standard_time_daylight) {
                    their_std_offset = -rule.ttis[type].gmt_offset;
                }
            }

            s64 their_offset{their_std_offset};
            for (int index{}; index < rule.time_count; ++index) {
                const s8 type{rule.types[index]};
                rule.types[index] = rule.ttis[type].is_dst ? 1 : 0;
                if (!rule.ttis[type].is_gmt) {
                    if (!rule.ttis[type].is_standard_time_daylight) {
                        rule.ats[index] += dest_offset - their_std_offset;
                    } else {
                        rule.ats[index] += std_offset - their_std_offset;
                    }
                }
                their_offset = -rule.ttis[type].gmt_offset;
                if (!rule.ttis[type].is_dst) {
                    their_std_offset = their_offset;
                }
            }
            rule.ttis[0].gmt_offset = -std_offset;
            rule.ttis[0].is_dst = false;
            rule.ttis[0].abbreviation_list_index = 0;
            rule.ttis[1].gmt_offset = -dest_offset;
            rule.ttis[1].is_dst = true;
            rule.ttis[1].abbreviation_list_index = std_len + 1;
            rule.type_count = 2;
            rule.default_type = 0;
        }
    } else {
        // Default is standard time
        rule.type_count = 1;
        rule.time_count = 0;
        rule.default_type = 0;
        rule.ttis[0].gmt_offset = -std_offset;
        rule.ttis[0].is_dst = false;
        rule.ttis[0].abbreviation_list_index = 0;
    }

    rule.char_count = char_count;
    for (int index{}; index < std_len; ++index) {
        rule.chars[index] = std_name[index];
    }

    rule.chars[std_len++] = '\0';
    if (dest_len != 0) {
        for (int index{}; index < dest_len; ++index) {
            rule.chars[std_len + index] = dest_name[index];
        }
        rule.chars[std_len + dest_len] = '\0';
    }

    return true;
}

static bool ParseTimeZoneBinary(TimeZoneRule& time_zone_rule, FileSys::VirtualFile& vfs_file) {
    TzifHeader header{};
    if (vfs_file->ReadObject<TzifHeader>(&header) != sizeof(TzifHeader)) {
        return {};
    }

    constexpr s32 time_zone_max_leaps{50};
    constexpr s32 time_zone_max_chars{50};
    if (!(0 <= header.leap_count && header.leap_count < time_zone_max_leaps &&
          0 < header.type_count && header.type_count < s32(time_zone_rule.ttis.size()) &&
          0 <= header.time_count && header.time_count < s32(time_zone_rule.ats.size()) &&
          0 <= header.char_count && header.char_count < time_zone_max_chars &&
          (header.ttis_std_count == header.type_count || header.ttis_std_count == 0) &&
          (header.ttis_gmt_count == header.type_count || header.ttis_gmt_count == 0))) {
        return {};
    }
    time_zone_rule.time_count = header.time_count;
    time_zone_rule.type_count = header.type_count;
    time_zone_rule.char_count = header.char_count;

    int time_count{};
    u64 read_offset = sizeof(TzifHeader);
    for (int index{}; index < time_zone_rule.time_count; ++index) {
        s64_be at{};
        vfs_file->ReadObject<s64_be>(&at, read_offset);
        time_zone_rule.types[index] = 1;
        if (time_count != 0 && at <= time_zone_rule.ats[time_count - 1]) {
            if (at < time_zone_rule.ats[time_count - 1]) {
                return {};
            }
            time_zone_rule.types[index - 1] = 0;
            time_count--;
        }
        time_zone_rule.ats[time_count++] = at;
        read_offset += sizeof(s64_be);
    }
    time_count = 0;
    for (int index{}; index < time_zone_rule.time_count; ++index) {
        const u8 type{*vfs_file->ReadByte(read_offset)};
        read_offset += sizeof(u8);
        if (time_zone_rule.time_count <= type) {
            return {};
        }
        if (time_zone_rule.types[index] != 0) {
            time_zone_rule.types[time_count++] = type;
        }
    }
    time_zone_rule.time_count = time_count;
    for (int index{}; index < time_zone_rule.type_count; ++index) {
        TimeTypeInfo& ttis{time_zone_rule.ttis[index]};
        u32_be gmt_offset{};
        vfs_file->ReadObject<u32_be>(&gmt_offset, read_offset);
        read_offset += sizeof(u32_be);
        ttis.gmt_offset = gmt_offset;

        const u8 dst{*vfs_file->ReadByte(read_offset)};
        read_offset += sizeof(u8);
        if (dst >= 2) {
            return {};
        }
        ttis.is_dst = dst != 0;

        const s32 abbreviation_list_index{*vfs_file->ReadByte(read_offset)};
        read_offset += sizeof(u8);
        if (abbreviation_list_index >= time_zone_rule.char_count) {
            return {};
        }
        ttis.abbreviation_list_index = abbreviation_list_index;
    }

    vfs_file->ReadArray(time_zone_rule.chars.data(), time_zone_rule.char_count, read_offset);
    time_zone_rule.chars[time_zone_rule.char_count] = '\0';
    read_offset += time_zone_rule.char_count;
    for (int index{}; index < time_zone_rule.type_count; ++index) {
        if (header.ttis_std_count == 0) {
            time_zone_rule.ttis[index].is_standard_time_daylight = false;
        } else {
            const u8 dst{*vfs_file->ReadByte(read_offset)};
            read_offset += sizeof(u8);
            if (dst >= 2) {
                return {};
            }
            time_zone_rule.ttis[index].is_standard_time_daylight = dst != 0;
        }
    }

    for (int index{}; index < time_zone_rule.type_count; ++index) {
        if (header.ttis_std_count == 0) {
            time_zone_rule.ttis[index].is_gmt = false;
        } else {
            const u8 dst{*vfs_file->ReadByte(read_offset)};
            read_offset += sizeof(u8);
            if (dst >= 2) {
                return {};
            }
            time_zone_rule.ttis[index].is_gmt = dst != 0;
        }
    }

    const u64 position{(read_offset - sizeof(TzifHeader))};
    const s64 bytes_read = s64(vfs_file->GetSize() - sizeof(TzifHeader) - position);
    if (bytes_read < 0) {
        return {};
    }
    constexpr s32 time_zone_name_max{255};
    if (bytes_read > (time_zone_name_max + 1)) {
        return {};
    }

    std::array<char, time_zone_name_max + 1> temp_name{};
    vfs_file->ReadArray(temp_name.data(), bytes_read, read_offset);
    if (bytes_read > 2 && temp_name[0] == '\n' && temp_name[bytes_read - 1] == '\n' &&
        std::size_t(time_zone_rule.type_count) + 2 <= time_zone_rule.ttis.size()) {
        temp_name[bytes_read - 1] = '\0';

        std::array<char, time_zone_name_max> name{};
        std::memcpy(name.data(), temp_name.data() + 1, std::size_t(bytes_read - 1));

        TimeZoneRule temp_rule;
        if (ParsePosixName(name.data(), temp_rule)) {
            UNIMPLEMENTED();
        }
    }
    if (time_zone_rule.type_count == 0) {
        return {};
    }
    if (time_zone_rule.time_count > 1) {
        UNIMPLEMENTED();
    }

    s32 default_type{};

    for (default_type = 0; default_type < time_zone_rule.time_count; default_type++) {
        if (time_zone_rule.types[default_type] == 0) {
            break;
        }
    }

    default_type = default_type < time_zone_rule.time_count ? -1 : 0;
    if (default_type < 0 && time_zone_rule.time_count > 0 &&
        time_zone_rule.ttis[time_zone_rule.types[0]].is_dst) {
        default_type = time_zone_rule.types[0];
        while (--default_type >= 0) {
            if (!time_zone_rule.ttis[default_type].is_dst) {
                break;
            }
        }
    }
    if (default_type < 0) {
        default_type = 0;
        while (time_zone_rule.ttis[default_type].is_dst) {
            if (++default_type >= time_zone_rule.type_count) {
                default_type = 0;
                break;
            }
        }
    }
    time_zone_rule.default_type = default_type;
    return true;
}

static ResultCode CreateCalendarTime(s64 time, int gmt_offset, CalendarTimeInternal& calendar_time,
                                     CalendarAdditionalInfo& calendar_additional_info) {
    s64 year{epoch_year};
    s64 time_days{time / seconds_per_day};
    s64 remaining_seconds{time % seconds_per_day};
    while (time_days < 0 || time_days >= GetYearLengthInDays(year)) {
        s64 delta = time_days / days_per_leap_year;
        if (!delta) {
            delta = time_days < 0 ? -1 : 1;
        }
        s64 new_year{year};
        if (!SafeAdd(new_year, delta)) {
            return ERROR_OUT_OF_RANGE;
        }
        time_days -= (new_year - year) * days_per_normal_year;
        time_days -= GetLeapDaysFromYear(new_year - 1) - GetLeapDaysFromYear(year - 1);
        year = new_year;
    }

    s64 day_of_year{time_days};
    remaining_seconds += gmt_offset;
    while (remaining_seconds < 0) {
        remaining_seconds += seconds_per_day;
        day_of_year--;
    }

    while (remaining_seconds >= seconds_per_day) {
        remaining_seconds -= seconds_per_day;
        day_of_year++;
    }

    while (day_of_year < 0) {
        if (!SafeAdd(year, -1)) {
            return ERROR_OUT_OF_RANGE;
        }
        day_of_year += GetYearLengthInDays(year);
    }

    while (day_of_year >= GetYearLengthInDays(year)) {
        day_of_year -= GetYearLengthInDays(year);
        if (!SafeAdd(year, 1)) {
            return ERROR_OUT_OF_RANGE;
        }
    }

    calendar_time.year = year;
    calendar_additional_info.day_of_year = static_cast<u32>(day_of_year);
    s64 day_of_week{
        (epoch_week_day +
         ((year - epoch_year) % days_per_week) * (days_per_normal_year % days_per_week) +
         GetLeapDaysFromYear(year - 1) - GetLeapDaysFromYear(epoch_year - 1) + day_of_year) %
        days_per_week};
    if (day_of_week < 0) {
        day_of_week += days_per_week;
    }

    calendar_additional_info.day_of_week = static_cast<u32>(day_of_week);
    calendar_time.hour = static_cast<s8>((remaining_seconds / seconds_per_hour) % seconds_per_hour);
    remaining_seconds %= seconds_per_hour;
    calendar_time.minute = static_cast<s8>(remaining_seconds / seconds_per_minute);
    calendar_time.second = static_cast<s8>(remaining_seconds % seconds_per_minute);

    for (calendar_time.month = 0;
         day_of_year >= GetMonthLength(IsLeapYear(year), calendar_time.month);
         ++calendar_time.month) {
        day_of_year -= GetMonthLength(IsLeapYear(year), calendar_time.month);
    }

    calendar_time.day = static_cast<s8>(day_of_year + 1);
    calendar_additional_info.is_dst = false;
    calendar_additional_info.gmt_offset = gmt_offset;

    return ResultSuccess;
}

static ResultCode ToCalendarTimeInternal(const TimeZoneRule& rules, s64 time,
                                         CalendarTimeInternal& calendar_time,
                                         CalendarAdditionalInfo& calendar_additional_info) {
    if ((rules.go_ahead && time < rules.ats[0]) ||
        (rules.go_back && time > rules.ats[rules.time_count - 1])) {
        s64 seconds{};
        if (time < rules.ats[0]) {
            seconds = rules.ats[0] - time;
        } else {
            seconds = time - rules.ats[rules.time_count - 1];
        }
        seconds--;

        const s64 years{(seconds / seconds_per_repeat + 1) * years_per_repeat};
        seconds = years * average_seconds_per_year;

        s64 new_time{time};
        if (time < rules.ats[0]) {
            new_time += seconds;
        } else {
            new_time -= seconds;
        }
        if (new_time < rules.ats[0] && new_time > rules.ats[rules.time_count - 1]) {
            return ERROR_TIME_NOT_FOUND;
        }
        if (const ResultCode result{
                ToCalendarTimeInternal(rules, new_time, calendar_time, calendar_additional_info)};
            result != ResultSuccess) {
            return result;
        }
        if (time < rules.ats[0]) {
            calendar_time.year -= years;
        } else {
            calendar_time.year += years;
        }

        return ResultSuccess;
    }

    s32 tti_index{};
    if (rules.time_count == 0 || time < rules.ats[0]) {
        tti_index = rules.default_type;
    } else {
        s32 low{1};
        s32 high{rules.time_count};
        while (low < high) {
            s32 mid{(low + high) >> 1};
            if (time < rules.ats[mid]) {
                high = mid;
            } else {
                low = mid + 1;
            }
        }
        tti_index = rules.types[low - 1];
    }

    if (const ResultCode result{CreateCalendarTime(time, rules.ttis[tti_index].gmt_offset,
                                                   calendar_time, calendar_additional_info)};
        result != ResultSuccess) {
        return result;
    }

    calendar_additional_info.is_dst = rules.ttis[tti_index].is_dst;
    const char* time_zone{&rules.chars[rules.ttis[tti_index].abbreviation_list_index]};
    for (int index{}; time_zone[index] != '\0'; ++index) {
        calendar_additional_info.timezone_name[index] = time_zone[index];
    }
    return ResultSuccess;
}

static ResultCode ToCalendarTimeImpl(const TimeZoneRule& rules, s64 time, CalendarInfo& calendar) {
    CalendarTimeInternal calendar_time{};
    const ResultCode result{
        ToCalendarTimeInternal(rules, time, calendar_time, calendar.additional_info)};
    calendar.time.year = static_cast<s16>(calendar_time.year);

    // Internal impl. uses 0-indexed month
    calendar.time.month = static_cast<s8>(calendar_time.month + 1);

    calendar.time.day = calendar_time.day;
    calendar.time.hour = calendar_time.hour;
    calendar.time.minute = calendar_time.minute;
    calendar.time.second = calendar_time.second;
    return result;
}

TimeZoneManager::TimeZoneManager() = default;
TimeZoneManager::~TimeZoneManager() = default;

ResultCode TimeZoneManager::ToCalendarTime(const TimeZoneRule& rules, s64 time,
                                           CalendarInfo& calendar) const {
    return ToCalendarTimeImpl(rules, time, calendar);
}

ResultCode TimeZoneManager::SetDeviceLocationNameWithTimeZoneRule(const std::string& location_name,
                                                                  FileSys::VirtualFile& vfs_file) {
    TimeZoneRule rule{};
    if (ParseTimeZoneBinary(rule, vfs_file)) {
        device_location_name = location_name;
        time_zone_rule = rule;
        return ResultSuccess;
    }
    return ERROR_TIME_ZONE_CONVERSION_FAILED;
}

ResultCode TimeZoneManager::SetUpdatedTime(const Clock::SteadyClockTimePoint& value) {
    time_zone_update_time_point = value;
    return ResultSuccess;
}

ResultCode TimeZoneManager::ToCalendarTimeWithMyRules(s64 time, CalendarInfo& calendar) const {
    if (is_initialized) {
        return ToCalendarTime(time_zone_rule, time, calendar);
    } else {
        return ERROR_UNINITIALIZED_CLOCK;
    }
}

ResultCode TimeZoneManager::ParseTimeZoneRuleBinary(TimeZoneRule& rules,
                                                    FileSys::VirtualFile& vfs_file) const {
    if (!ParseTimeZoneBinary(rules, vfs_file)) {
        return ERROR_TIME_ZONE_CONVERSION_FAILED;
    }
    return ResultSuccess;
}

ResultCode TimeZoneManager::ToPosixTime(const TimeZoneRule& rules,
                                        const CalendarTime& calendar_time, s64& posix_time) const {
    posix_time = 0;

    CalendarTimeInternal internal_time{
        .year = calendar_time.year,
        // Internal impl. uses 0-indexed month
        .month = static_cast<s8>(calendar_time.month - 1),
        .day = calendar_time.day,
        .hour = calendar_time.hour,
        .minute = calendar_time.minute,
        .second = calendar_time.second,
    };

    s32 hour{internal_time.hour};
    s32 minute{internal_time.minute};
    if (!SafeNormalize(hour, minute, minutes_per_hour)) {
        return ERROR_OVERFLOW;
    }
    internal_time.minute = static_cast<s8>(minute);

    s32 day{internal_time.day};
    if (!SafeNormalize(day, hour, hours_per_day)) {
        return ERROR_OVERFLOW;
    }
    internal_time.day = static_cast<s8>(day);
    internal_time.hour = static_cast<s8>(hour);

    s64 year{internal_time.year};
    s64 month{internal_time.month};
    if (!SafeNormalize(year, month, months_per_year)) {
        return ERROR_OVERFLOW;
    }
    internal_time.month = static_cast<s8>(month);

    if (!SafeAdd(year, year_base)) {
        return ERROR_OVERFLOW;
    }

    while (day <= 0) {
        if (!SafeAdd(year, -1)) {
            return ERROR_OVERFLOW;
        }
        s64 temp_year{year};
        if (1 < internal_time.month) {
            ++temp_year;
        }
        day += static_cast<s32>(GetYearLengthInDays(temp_year));
    }

    while (day > days_per_leap_year) {
        s64 temp_year{year};
        if (1 < internal_time.month) {
            temp_year++;
        }
        day -= static_cast<s32>(GetYearLengthInDays(temp_year));
        if (!SafeAdd(year, 1)) {
            return ERROR_OVERFLOW;
        }
    }

    while (true) {
        const s32 month_length{GetMonthLength(IsLeapYear(year), internal_time.month)};
        if (day <= month_length) {
            break;
        }
        day -= month_length;
        internal_time.month++;
        if (internal_time.month >= months_per_year) {
            internal_time.month = 0;
            if (!SafeAdd(year, 1)) {
                return ERROR_OVERFLOW;
            }
        }
    }
    internal_time.day = static_cast<s8>(day);

    if (!SafeAdd(year, -year_base)) {
        return ERROR_OVERFLOW;
    }
    internal_time.year = year;

    s32 saved_seconds{};
    if (internal_time.second >= 0 && internal_time.second < seconds_per_minute) {
        saved_seconds = 0;
    } else if (year + year_base < epoch_year) {
        s32 second{internal_time.second};
        if (!SafeAdd(second, 1 - seconds_per_minute)) {
            return ERROR_OVERFLOW;
        }
        saved_seconds = second;
        internal_time.second = 1 - seconds_per_minute;
    } else {
        saved_seconds = internal_time.second;
        internal_time.second = 0;
    }

    s64 low{LLONG_MIN};
    s64 high{LLONG_MAX};
    while (true) {
        s64 pivot{low / 2 + high / 2};
        if (pivot < low) {
            pivot = low;
        } else if (pivot > high) {
            pivot = high;
        }
        s32 direction{};
        CalendarTimeInternal candidate_calendar_time{};
        CalendarAdditionalInfo unused{};
        if (ToCalendarTimeInternal(rules, pivot, candidate_calendar_time, unused) !=
            ResultSuccess) {
            if (pivot > 0) {
                direction = 1;
            } else {
                direction = -1;
            }
        } else {
            direction = candidate_calendar_time.Compare(internal_time);
        }
        if (!direction) {
            const s64 time_result{pivot + saved_seconds};
            if ((time_result < pivot) != (saved_seconds < 0)) {
                return ERROR_OVERFLOW;
            }
            posix_time = time_result;
            break;
        } else {
            if (pivot == low) {
                if (pivot == LLONG_MAX) {
                    return ERROR_TIME_NOT_FOUND;
                }
                pivot++;
                low++;
            } else if (pivot == high) {
                if (pivot == LLONG_MIN) {
                    return ERROR_TIME_NOT_FOUND;
                }
                pivot--;
                high--;
            }
            if (low > high) {
                return ERROR_TIME_NOT_FOUND;
            }
            if (direction > 0) {
                high = pivot;
            } else {
                low = pivot;
            }
        }
    }
    return ResultSuccess;
}

ResultCode TimeZoneManager::ToPosixTimeWithMyRule(const CalendarTime& calendar_time,
                                                  s64& posix_time) const {
    if (is_initialized) {
        return ToPosixTime(time_zone_rule, calendar_time, posix_time);
    }
    posix_time = 0;
    return ERROR_UNINITIALIZED_CLOCK;
}

ResultCode TimeZoneManager::GetDeviceLocationName(LocationName& value) const {
    if (!is_initialized) {
        return ERROR_UNINITIALIZED_CLOCK;
    }
    std::memcpy(value.data(), device_location_name.c_str(), device_location_name.size());
    return ResultSuccess;
}

} // namespace Service::Time::TimeZone
