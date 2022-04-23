// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::HID {

constexpr ResultCode NpadInvalidHandle{ErrorModule::HID, 100};
constexpr ResultCode InvalidSixAxisFusionRange{ErrorModule::HID, 423};
constexpr ResultCode NpadNotConnected{ErrorModule::HID, 710};

} // namespace Service::HID
