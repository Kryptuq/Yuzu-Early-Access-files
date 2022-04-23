// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Kernel {

// Confirmed Switch kernel error codes

constexpr ResultCode ResultOutOfSessions{ErrorModule::Kernel, 7};
constexpr ResultCode ResultInvalidArgument{ErrorModule::Kernel, 14};
constexpr ResultCode ResultNoSynchronizationObject{ErrorModule::Kernel, 57};
constexpr ResultCode ResultTerminationRequested{ErrorModule::Kernel, 59};
constexpr ResultCode ResultInvalidSize{ErrorModule::Kernel, 101};
constexpr ResultCode ResultInvalidAddress{ErrorModule::Kernel, 102};
constexpr ResultCode ResultOutOfResource{ErrorModule::Kernel, 103};
constexpr ResultCode ResultOutOfMemory{ErrorModule::Kernel, 104};
constexpr ResultCode ResultOutOfHandles{ErrorModule::Kernel, 105};
constexpr ResultCode ResultInvalidCurrentMemory{ErrorModule::Kernel, 106};
constexpr ResultCode ResultInvalidNewMemoryPermission{ErrorModule::Kernel, 108};
constexpr ResultCode ResultInvalidMemoryRegion{ErrorModule::Kernel, 110};
constexpr ResultCode ResultInvalidPriority{ErrorModule::Kernel, 112};
constexpr ResultCode ResultInvalidCoreId{ErrorModule::Kernel, 113};
constexpr ResultCode ResultInvalidHandle{ErrorModule::Kernel, 114};
constexpr ResultCode ResultInvalidPointer{ErrorModule::Kernel, 115};
constexpr ResultCode ResultInvalidCombination{ErrorModule::Kernel, 116};
constexpr ResultCode ResultTimedOut{ErrorModule::Kernel, 117};
constexpr ResultCode ResultCancelled{ErrorModule::Kernel, 118};
constexpr ResultCode ResultOutOfRange{ErrorModule::Kernel, 119};
constexpr ResultCode ResultInvalidEnumValue{ErrorModule::Kernel, 120};
constexpr ResultCode ResultNotFound{ErrorModule::Kernel, 121};
constexpr ResultCode ResultBusy{ErrorModule::Kernel, 122};
constexpr ResultCode ResultSessionClosed{ErrorModule::Kernel, 123};
constexpr ResultCode ResultInvalidState{ErrorModule::Kernel, 125};
constexpr ResultCode ResultReservedUsed{ErrorModule::Kernel, 126};
constexpr ResultCode ResultPortClosed{ErrorModule::Kernel, 131};
constexpr ResultCode ResultLimitReached{ErrorModule::Kernel, 132};
constexpr ResultCode ResultInvalidId{ErrorModule::Kernel, 519};

} // namespace Kernel
