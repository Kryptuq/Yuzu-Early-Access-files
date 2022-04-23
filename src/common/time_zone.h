// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <string>

namespace Common::TimeZone {

/// Gets the default timezone, i.e. "GMT"
[[nodiscard]] std::string GetDefaultTimeZone();

/// Gets the offset of the current timezone (from the default), in seconds
[[nodiscard]] std::chrono::seconds GetCurrentOffsetSeconds();

} // namespace Common::TimeZone
