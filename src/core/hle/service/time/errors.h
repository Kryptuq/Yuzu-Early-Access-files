// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Time {

constexpr Result ERROR_PERMISSION_DENIED{ErrorModule::Time, 1};
constexpr Result ERROR_TIME_MISMATCH{ErrorModule::Time, 102};
constexpr Result ERROR_UNINITIALIZED_CLOCK{ErrorModule::Time, 103};
constexpr Result ERROR_TIME_NOT_FOUND{ErrorModule::Time, 200};
constexpr Result ERROR_OVERFLOW{ErrorModule::Time, 201};
constexpr Result ERROR_LOCATION_NAME_TOO_LONG{ErrorModule::Time, 801};
constexpr Result ERROR_OUT_OF_RANGE{ErrorModule::Time, 902};
constexpr Result ERROR_TIME_ZONE_CONVERSION_FAILED{ErrorModule::Time, 903};
constexpr Result ERROR_TIME_ZONE_NOT_FOUND{ErrorModule::Time, 989};
constexpr Result ERROR_NOT_IMPLEMENTED{ErrorModule::Time, 990};

} // namespace Service::Time
