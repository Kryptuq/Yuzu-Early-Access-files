// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/global_scheduler_context.h"
#include "core/hle/kernel/k_hardware_timer.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

class [[nodiscard]] KScopedSchedulerLockAndSleep {
public:
    explicit KScopedSchedulerLockAndSleep(KernelCore& kernel_, KHardwareTimer** out_timer,
                                          KThread* t, s64 timeout)
        : kernel(kernel_), timeout_tick(timeout), thread(t), timer() {
        // Lock the scheduler.
        kernel.GlobalSchedulerContext().scheduler_lock.Lock();

        // Set our timer only if the time is positive.
        timer = (timeout_tick > 0) ? std::addressof(kernel.HardwareTimer()) : nullptr;

        *out_timer = timer;
    }

    ~KScopedSchedulerLockAndSleep() {
        // Register the sleep.
        if (timeout_tick > 0) {
            timer->RegisterTask(thread, timeout_tick);
        }

        // Unlock the scheduler.
        kernel.GlobalSchedulerContext().scheduler_lock.Unlock();
    }

    void CancelSleep() {
        timeout_tick = 0;
    }

private:
    KernelCore& kernel;
    s64 timeout_tick{};
    KThread* thread{};
    KHardwareTimer* timer{};
};

} // namespace Kernel
