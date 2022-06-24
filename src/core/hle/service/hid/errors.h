// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::HID {

constexpr ResultCode NpadInvalidHandle{ErrorModule::HID, 100};
constexpr ResultCode NpadDeviceIndexOutOfRange{ErrorModule::HID, 107};
constexpr ResultCode VibrationInvalidStyleIndex{ErrorModule::HID, 122};
constexpr ResultCode VibrationInvalidNpadId{ErrorModule::HID, 123};
constexpr ResultCode VibrationDeviceIndexOutOfRange{ErrorModule::HID, 124};
constexpr ResultCode InvalidSixAxisFusionRange{ErrorModule::HID, 423};
constexpr ResultCode NpadIsDualJoycon{ErrorModule::HID, 601};
constexpr ResultCode NpadIsSameType{ErrorModule::HID, 602};
constexpr ResultCode InvalidNpadId{ErrorModule::HID, 709};
constexpr ResultCode NpadNotConnected{ErrorModule::HID, 710};

} // namespace Service::HID
