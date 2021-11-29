// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/k_thread.h"

namespace Kernel {

class KernelCore;
class KLightLock;

class KLightConditionVariable {
public:
    explicit KLightConditionVariable(KernelCore& kernel_) : kernel{kernel_} {}

    void Wait(KLightLock* lock, s64 timeout = -1, bool allow_terminating_thread = true);
    void Broadcast();

private:
    KernelCore& kernel;
    KThread::WaiterList wait_list{};
};
} // namespace Kernel
