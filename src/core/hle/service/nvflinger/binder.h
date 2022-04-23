// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2014 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/binder/IBinder.h

#pragma once

#include "common/common_types.h"

namespace Kernel {
class HLERequestContext;
class KReadableEvent;
} // namespace Kernel

namespace Service::android {

enum class TransactionId {
    RequestBuffer = 1,
    SetBufferCount = 2,
    DequeueBuffer = 3,
    DetachBuffer = 4,
    DetachNextBuffer = 5,
    AttachBuffer = 6,
    QueueBuffer = 7,
    CancelBuffer = 8,
    Query = 9,
    Connect = 10,
    Disconnect = 11,
    AllocateBuffers = 13,
    SetPreallocatedBuffer = 14,
    GetBufferHistory = 17,
};

class IBinder {
public:
    virtual void Transact(Kernel::HLERequestContext& ctx, android::TransactionId code,
                          u32 flags) = 0;
    virtual Kernel::KReadableEvent& GetNativeHandle() = 0;
};

} // namespace Service::android
