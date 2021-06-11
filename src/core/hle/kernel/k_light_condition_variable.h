// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// This file references various implementation details from Atmosphere, an open-source firmware for
// the Nintendo Switch. Copyright 2018-2020 Atmosphere-NX.

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/k_thread_queue.h"
#include "core/hle/kernel/time_manager.h"

namespace Kernel {
class KernelCore;

class KLightConditionVariable {
public:
    explicit KLightConditionVariable(KernelCore& kernel_)
        : thread_queue(kernel_), kernel(kernel_) {}

    void Wait(KLightLock* lock, s64 timeout = -1) {
        WaitImpl(lock, timeout);
        lock->Lock();
    }

    void Broadcast() {
        KScopedSchedulerLock lk{kernel};
        while (thread_queue.WakeupFrontThread() != nullptr) {
            // We want to signal all threads, and so should continue waking up until there's nothing
            // to wake.
        }
    }

private:
    void WaitImpl(KLightLock* lock, s64 timeout) {
        KThread* owner = GetCurrentThreadPointer(kernel);

        // Sleep the thread.
        {
            KScopedSchedulerLockAndSleep lk(kernel, owner, timeout);
            lock->Unlock();

            if (!thread_queue.SleepThread(owner)) {
                lk.CancelSleep();
                return;
            }
        }

        // Cancel the task that the sleep setup.
        kernel.TimeManager().UnscheduleTimeEvent(owner);
    }
    KThreadQueue thread_queue;
    KernelCore& kernel;
};
} // namespace Kernel
