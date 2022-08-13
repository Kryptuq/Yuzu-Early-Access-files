// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::LDN {

constexpr Result ResultAdvertiseDataTooLarge{ErrorModule::LDN, 10};
constexpr Result ResultAuthenticationFailed{ErrorModule::LDN, 20};
constexpr Result ResultDisabled{ErrorModule::LDN, 22};
constexpr Result ResultAirplaneModeEnabled{ErrorModule::LDN, 23};
constexpr Result ResultInvalidNodeCount{ErrorModule::LDN, 30};
constexpr Result ResultConnectionFailed{ErrorModule::LDN, 31};
constexpr Result ResultBadState{ErrorModule::LDN, 32};
constexpr Result ResultNoIpAddress{ErrorModule::LDN, 33};
constexpr Result ResultInvalidBufferCount{ErrorModule::LDN, 50};
constexpr Result ResultAccessPointConnectionFailed{ErrorModule::LDN, 65};
constexpr Result ResultAuthenticationTimeout{ErrorModule::LDN, 66};
constexpr Result ResultMaximumNodeCount{ErrorModule::LDN, 67};
constexpr Result ResultBadInput{ErrorModule::LDN, 96};
constexpr Result ResultLocalCommunicationIdNotFound{ErrorModule::LDN, 97};
constexpr Result ResultLocalCommunicationVersionTooLow{ErrorModule::LDN, 113};
constexpr Result ResultLocalCommunicationVersionTooHigh{ErrorModule::LDN, 114};

} // namespace Service::LDN
