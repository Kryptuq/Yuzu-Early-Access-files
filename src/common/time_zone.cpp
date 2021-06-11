// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include <iomanip>
#include <sstream>

#include "common/logging/log.h"
#include "common/time_zone.h"

namespace Common::TimeZone {

std::string GetDefaultTimeZone() {
    return "GMT";
}

static std::string GetOsTimeZoneOffset() {
    const std::time_t t{std::time(nullptr)};
    const std::tm tm{*std::localtime(&t)};

    std::stringstream ss;
    ss << std::put_time(&tm, "%z"); // Get the current timezone offset, e.g. "-400", as a string

    return ss.str();
}

static int ConvertOsTimeZoneOffsetToInt(const std::string& timezone) {
    try {
        return std::stoi(timezone);
    } catch (const std::invalid_argument&) {
        LOG_CRITICAL(Common, "invalid_argument with {}!", timezone);
        return 0;
    } catch (const std::out_of_range&) {
        LOG_CRITICAL(Common, "out_of_range with {}!", timezone);
        return 0;
    }
}

std::chrono::seconds GetCurrentOffsetSeconds() {
    const int offset{ConvertOsTimeZoneOffsetToInt(GetOsTimeZoneOffset())};

    int seconds{(offset / 100) * 60 * 60}; // Convert hour component to seconds
    seconds += (offset % 100) * 60;        // Convert minute component to seconds

    return std::chrono::seconds{seconds};
}

} // namespace Common::TimeZone
