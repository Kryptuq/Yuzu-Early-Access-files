// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// This file references various implementation details from Atmosphere, an open-source firmware for
// the Nintendo Switch. Copyright 2018-2020 Atmosphere-NX.

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/time_manager.h"

namespace Kernel {
class KernelCore;

class KLightConditionVariable {
public:
    explicit KLightConditionVariable(KernelCore& kernel_) : kernel{kernel_} {}

    void Wait(KLightLock* lock, s64 timeout = -1, bool allow_terminating_thread = true) {
        WaitImpl(lock, timeout, allow_terminating_thread);
    }

    void Broadcast() {
        KScopedSchedulerLock lk{kernel};

        // Signal all threads.
        for (auto& thread : wait_list) {
            thread.SetState(ThreadState::Runnable);
        }
    }

private:
    void WaitImpl(KLightLock* lock, s64 timeout, bool allow_terminating_thread) {
        KThread* owner = GetCurrentThreadPointer(kernel);

        // Sleep the thread.
        {
            KScopedSchedulerLockAndSleep lk{kernel, owner, timeout};

            if (!allow_terminating_thread && owner->IsTerminationRequested()) {
                lk.CancelSleep();
                return;
            }

            lock->Unlock();

            // Set the thread as waiting.
            GetCurrentThread(kernel).SetState(ThreadState::Waiting);

            // Add the thread to the queue.
            wait_list.push_back(GetCurrentThread(kernel));
        }

        // Remove the thread from the wait list.
        {
            KScopedSchedulerLock sl{kernel};

            wait_list.erase(wait_list.iterator_to(GetCurrentThread(kernel)));
        }

        // Cancel the task that the sleep setup.
        kernel.TimeManager().UnscheduleTimeEvent(owner);

        // Re-acquire the lock.
        lock->Lock();
    }

    KernelCore& kernel;
    KThread::WaiterList wait_list{};
};
} // namespace Kernel
