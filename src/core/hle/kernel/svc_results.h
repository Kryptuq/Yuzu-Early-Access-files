// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Kernel {

// Confirmed Switch kernel error codes

constexpr Result ResultOutOfSessions{ErrorModule::Kernel, 7};
constexpr Result ResultInvalidArgument{ErrorModule::Kernel, 14};
constexpr Result ResultNotImplemented{ErrorModule::Kernel, 33};
constexpr Result ResultNoSynchronizationObject{ErrorModule::Kernel, 57};
constexpr Result ResultTerminationRequested{ErrorModule::Kernel, 59};
constexpr Result ResultInvalidSize{ErrorModule::Kernel, 101};
constexpr Result ResultInvalidAddress{ErrorModule::Kernel, 102};
constexpr Result ResultOutOfResource{ErrorModule::Kernel, 103};
constexpr Result ResultOutOfMemory{ErrorModule::Kernel, 104};
constexpr Result ResultOutOfHandles{ErrorModule::Kernel, 105};
constexpr Result ResultInvalidCurrentMemory{ErrorModule::Kernel, 106};
constexpr Result ResultInvalidNewMemoryPermission{ErrorModule::Kernel, 108};
constexpr Result ResultInvalidMemoryRegion{ErrorModule::Kernel, 110};
constexpr Result ResultInvalidPriority{ErrorModule::Kernel, 112};
constexpr Result ResultInvalidCoreId{ErrorModule::Kernel, 113};
constexpr Result ResultInvalidHandle{ErrorModule::Kernel, 114};
constexpr Result ResultInvalidPointer{ErrorModule::Kernel, 115};
constexpr Result ResultInvalidCombination{ErrorModule::Kernel, 116};
constexpr Result ResultTimedOut{ErrorModule::Kernel, 117};
constexpr Result ResultCancelled{ErrorModule::Kernel, 118};
constexpr Result ResultOutOfRange{ErrorModule::Kernel, 119};
constexpr Result ResultInvalidEnumValue{ErrorModule::Kernel, 120};
constexpr Result ResultNotFound{ErrorModule::Kernel, 121};
constexpr Result ResultBusy{ErrorModule::Kernel, 122};
constexpr Result ResultSessionClosed{ErrorModule::Kernel, 123};
constexpr Result ResultInvalidState{ErrorModule::Kernel, 125};
constexpr Result ResultReservedUsed{ErrorModule::Kernel, 126};
constexpr Result ResultPortClosed{ErrorModule::Kernel, 131};
constexpr Result ResultLimitReached{ErrorModule::Kernel, 132};
constexpr Result ResultOutOfAddressSpace{ErrorModule::Kernel, 259};
constexpr Result ResultInvalidId{ErrorModule::Kernel, 519};

} // namespace Kernel
