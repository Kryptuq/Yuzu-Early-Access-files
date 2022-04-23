// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::SPL {

// Description 0 - 99
constexpr ResultCode ResultSecureMonitorError{ErrorModule::SPL, 0};
constexpr ResultCode ResultSecureMonitorNotImplemented{ErrorModule::SPL, 1};
constexpr ResultCode ResultSecureMonitorInvalidArgument{ErrorModule::SPL, 2};
constexpr ResultCode ResultSecureMonitorBusy{ErrorModule::SPL, 3};
constexpr ResultCode ResultSecureMonitorNoAsyncOperation{ErrorModule::SPL, 4};
constexpr ResultCode ResultSecureMonitorInvalidAsyncOperation{ErrorModule::SPL, 5};
constexpr ResultCode ResultSecureMonitorNotPermitted{ErrorModule::SPL, 6};
constexpr ResultCode ResultSecureMonitorNotInitialized{ErrorModule::SPL, 7};

constexpr ResultCode ResultInvalidSize{ErrorModule::SPL, 100};
constexpr ResultCode ResultUnknownSecureMonitorError{ErrorModule::SPL, 101};
constexpr ResultCode ResultDecryptionFailed{ErrorModule::SPL, 102};

constexpr ResultCode ResultOutOfKeySlots{ErrorModule::SPL, 104};
constexpr ResultCode ResultInvalidKeySlot{ErrorModule::SPL, 105};
constexpr ResultCode ResultBootReasonAlreadySet{ErrorModule::SPL, 106};
constexpr ResultCode ResultBootReasonNotSet{ErrorModule::SPL, 107};
constexpr ResultCode ResultInvalidArgument{ErrorModule::SPL, 108};

} // namespace Service::SPL
