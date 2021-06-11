// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <chrono>
#include <string>

namespace Common::TimeZone {

/// Gets the default timezone, i.e. "GMT"
[[nodiscard]] std::string GetDefaultTimeZone();

/// Gets the offset of the current timezone (from the default), in seconds
[[nodiscard]] std::chrono::seconds GetCurrentOffsetSeconds();

} // namespace Common::TimeZone
