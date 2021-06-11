// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// This file references various implementation details from Atmosphere, an open-source firmware for
// the Nintendo Switch. Copyright 2018-2020 Atmosphere-NX.

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/k_handle_table.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/time_manager.h"

namespace Kernel {

class [[nodiscard]] KScopedSchedulerLockAndSleep {
public:
    explicit KScopedSchedulerLockAndSleep(KernelCore & kernel_, KThread * t, s64 timeout)
        : kernel(kernel_), thread(t), timeout_tick(timeout) {
        // Lock the scheduler.
        kernel.GlobalSchedulerContext().scheduler_lock.Lock();
    }

    ~KScopedSchedulerLockAndSleep() {
        // Register the sleep.
        if (timeout_tick > 0) {
            kernel.TimeManager().ScheduleTimeEvent(thread, timeout_tick);
        }

        // Unlock the scheduler.
        kernel.GlobalSchedulerContext().scheduler_lock.Unlock();
    }

    void CancelSleep() {
        timeout_tick = 0;
    }

private:
    KernelCore& kernel;
    KThread* thread{};
    s64 timeout_tick{};
};

} // namespace Kernel
