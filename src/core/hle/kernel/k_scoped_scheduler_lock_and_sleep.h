// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/global_scheduler_context.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/time_manager.h"

namespace Kernel {

class [[nodiscard]] KScopedSchedulerLockAndSleep {
public:
    explicit KScopedSchedulerLockAndSleep(KernelCore& kernel_, KThread* t, s64 timeout)
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
