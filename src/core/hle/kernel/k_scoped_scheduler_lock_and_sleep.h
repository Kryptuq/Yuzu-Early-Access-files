// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/global_scheduler_context.h"
#include "core/hle/kernel/k_hardware_timer.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

class KScopedSchedulerLockAndSleep {
public:
    explicit KScopedSchedulerLockAndSleep(KernelCore& kernel, KHardwareTimer** out_timer,
                                          KThread* thread, s64 timeout_tick)
        : m_kernel(kernel), m_timeout_tick(timeout_tick), m_thread(thread), m_timer() {
        // Lock the scheduler.
        kernel.GlobalSchedulerContext().m_scheduler_lock.Lock();

        // Set our timer only if the time is positive.
        m_timer = (timeout_tick > 0) ? std::addressof(kernel.HardwareTimer()) : nullptr;

        *out_timer = m_timer;
    }

    ~KScopedSchedulerLockAndSleep() {
        // Register the sleep.
        if (m_timeout_tick > 0) {
            m_timer->RegisterTask(m_thread, m_timeout_tick);
        }

        // Unlock the scheduler.
        m_kernel.GlobalSchedulerContext().m_scheduler_lock.Unlock();
    }

    void CancelSleep() {
        m_timeout_tick = 0;
    }

private:
    KernelCore& m_kernel;
    s64 m_timeout_tick{};
    KThread* m_thread{};
    KHardwareTimer* m_timer{};
};

} // namespace Kernel
