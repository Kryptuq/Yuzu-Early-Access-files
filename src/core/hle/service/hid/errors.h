// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace Service::HID {

constexpr ResultCode ERR_NPAD_NOT_CONNECTED{ErrorModule::HID, 710};

} // namespace Service::HID
