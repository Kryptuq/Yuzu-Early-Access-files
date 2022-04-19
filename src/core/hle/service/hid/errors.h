// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace Service::HID {

constexpr ResultCode NpadInvalidHandle{ErrorModule::HID, 100};
constexpr ResultCode InvalidSixAxisFusionRange{ErrorModule::HID, 423};
constexpr ResultCode NpadNotConnected{ErrorModule::HID, 710};

} // namespace Service::HID
