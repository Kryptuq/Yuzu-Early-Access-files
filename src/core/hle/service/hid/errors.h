// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::HID {

constexpr Result PalmaResultSuccess{ErrorModule::HID, 0};
constexpr Result NpadInvalidHandle{ErrorModule::HID, 100};
constexpr Result NpadDeviceIndexOutOfRange{ErrorModule::HID, 107};
constexpr Result VibrationInvalidStyleIndex{ErrorModule::HID, 122};
constexpr Result VibrationInvalidNpadId{ErrorModule::HID, 123};
constexpr Result VibrationDeviceIndexOutOfRange{ErrorModule::HID, 124};
constexpr Result InvalidSixAxisFusionRange{ErrorModule::HID, 423};
constexpr Result NpadIsDualJoycon{ErrorModule::HID, 601};
constexpr Result NpadIsSameType{ErrorModule::HID, 602};
constexpr Result InvalidNpadId{ErrorModule::HID, 709};
constexpr Result NpadNotConnected{ErrorModule::HID, 710};
constexpr Result InvalidArraySize{ErrorModule::HID, 715};
constexpr Result InvalidPalmaHandle{ErrorModule::HID, 3302};

} // namespace Service::HID

namespace Service::IRS {

constexpr Result InvalidProcessorState{ErrorModule::Irsensor, 78};
constexpr Result InvalidIrCameraHandle{ErrorModule::Irsensor, 204};

} // namespace Service::IRS
