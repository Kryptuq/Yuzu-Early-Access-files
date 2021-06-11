// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace Service::Audio {

constexpr ResultCode ERR_OPERATION_FAILED{ErrorModule::Audio, 2};
constexpr ResultCode ERR_BUFFER_COUNT_EXCEEDED{ErrorModule::Audio, 8};
constexpr ResultCode ERR_NOT_SUPPORTED{ErrorModule::Audio, 513};

} // namespace Service::Audio
