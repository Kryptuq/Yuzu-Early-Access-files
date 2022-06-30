// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::SPL {

// Description 0 - 99
constexpr Result ResultSecureMonitorError{ErrorModule::SPL, 0};
constexpr Result ResultSecureMonitorNotImplemented{ErrorModule::SPL, 1};
constexpr Result ResultSecureMonitorInvalidArgument{ErrorModule::SPL, 2};
constexpr Result ResultSecureMonitorBusy{ErrorModule::SPL, 3};
constexpr Result ResultSecureMonitorNoAsyncOperation{ErrorModule::SPL, 4};
constexpr Result ResultSecureMonitorInvalidAsyncOperation{ErrorModule::SPL, 5};
constexpr Result ResultSecureMonitorNotPermitted{ErrorModule::SPL, 6};
constexpr Result ResultSecureMonitorNotInitialized{ErrorModule::SPL, 7};

constexpr Result ResultInvalidSize{ErrorModule::SPL, 100};
constexpr Result ResultUnknownSecureMonitorError{ErrorModule::SPL, 101};
constexpr Result ResultDecryptionFailed{ErrorModule::SPL, 102};

constexpr Result ResultOutOfKeySlots{ErrorModule::SPL, 104};
constexpr Result ResultInvalidKeySlot{ErrorModule::SPL, 105};
constexpr Result ResultBootReasonAlreadySet{ErrorModule::SPL, 106};
constexpr Result ResultBootReasonNotSet{ErrorModule::SPL, 107};
constexpr Result ResultInvalidArgument{ErrorModule::SPL, 108};

} // namespace Service::SPL
