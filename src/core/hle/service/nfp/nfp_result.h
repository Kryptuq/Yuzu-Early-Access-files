// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::NFP {

constexpr Result DeviceNotFound(ErrorModule::NFP, 64);
constexpr Result WrongDeviceState(ErrorModule::NFP, 73);
constexpr Result NfcDisabled(ErrorModule::NFP, 80);
constexpr Result WriteAmiiboFailed(ErrorModule::NFP, 88);
constexpr Result TagRemoved(ErrorModule::NFP, 97);
constexpr Result RegistrationIsNotInitialized(ErrorModule::NFP, 120);
constexpr Result ApplicationAreaIsNotInitialized(ErrorModule::NFP, 128);
constexpr Result CorruptedData(ErrorModule::NFP, 144);
constexpr Result WrongApplicationAreaId(ErrorModule::NFP, 152);
constexpr Result ApplicationAreaExist(ErrorModule::NFP, 168);
constexpr Result NotAnAmiibo(ErrorModule::NFP, 178);

} // namespace Service::NFP
