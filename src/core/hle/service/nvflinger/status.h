// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Service::android {

enum class Status : s32 {
    None = 0,
    NoError = 0,
    StaleBufferSlot = 1,
    NoBufferAvailable = 2,
    PresentLater = 3,
    WouldBlock = -11,
    NoMemory = -12,
    Busy = -16,
    NoInit = -19,
    BadValue = -22,
    InvalidOperation = -37,
    BufferNeedsReallocation = 1,
    ReleaseAllBuffers = 2,
};
DECLARE_ENUM_FLAG_OPERATORS(Status);

} // namespace Service::android
