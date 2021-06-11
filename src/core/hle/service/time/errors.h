// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace Service::Time {

constexpr ResultCode ERROR_PERMISSION_DENIED{ErrorModule::Time, 1};
constexpr ResultCode ERROR_TIME_MISMATCH{ErrorModule::Time, 102};
constexpr ResultCode ERROR_UNINITIALIZED_CLOCK{ErrorModule::Time, 103};
constexpr ResultCode ERROR_TIME_NOT_FOUND{ErrorModule::Time, 200};
constexpr ResultCode ERROR_OVERFLOW{ErrorModule::Time, 201};
constexpr ResultCode ERROR_LOCATION_NAME_TOO_LONG{ErrorModule::Time, 801};
constexpr ResultCode ERROR_OUT_OF_RANGE{ErrorModule::Time, 902};
constexpr ResultCode ERROR_TIME_ZONE_CONVERSION_FAILED{ErrorModule::Time, 903};
constexpr ResultCode ERROR_TIME_ZONE_NOT_FOUND{ErrorModule::Time, 989};
constexpr ResultCode ERROR_NOT_IMPLEMENTED{ErrorModule::Time, 990};

} // namespace Service::Time
