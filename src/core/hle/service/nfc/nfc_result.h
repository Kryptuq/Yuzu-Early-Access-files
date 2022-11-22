// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::NFC {

constexpr Result DeviceNotFound(ErrorModule::NFC, 64);
constexpr Result InvalidArgument(ErrorModule::NFC, 65);
constexpr Result WrongDeviceState(ErrorModule::NFC, 73);
constexpr Result NfcDisabled(ErrorModule::NFC, 80);
constexpr Result TagRemoved(ErrorModule::NFC, 97);
constexpr Result CorruptedData(ErrorModule::NFC, 144);

} // namespace Service::NFC
