// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/k_thread.h"

namespace Kernel {

class KThreadQueue {
public:
    explicit KThreadQueue(KernelCore& kernel_) : kernel{kernel_} {}

    bool IsEmpty() const {
        return wait_list.empty();
    }

    KThread::WaiterList::iterator begin() {
        return wait_list.begin();
    }
    KThread::WaiterList::iterator end() {
        return wait_list.end();
    }

    bool SleepThread(KThread* t) {
        KScopedSchedulerLock sl{kernel};

        // If the thread needs terminating, don't enqueue it.
        if (t->IsTerminationRequested()) {
            return false;
        }

        // Set the thread's queue and mark it as waiting.
        t->SetSleepingQueue(this);
        t->SetState(ThreadState::Waiting);

        // Add the thread to the queue.
        wait_list.push_back(*t);

        return true;
    }

    void WakeupThread(KThread* t) {
        KScopedSchedulerLock sl{kernel};

        // Remove the thread from the queue.
        wait_list.erase(wait_list.iterator_to(*t));

        // Mark the thread as no longer sleeping.
        t->SetState(ThreadState::Runnable);
        t->SetSleepingQueue(nullptr);
    }

    KThread* WakeupFrontThread() {
        KScopedSchedulerLock sl{kernel};

        if (wait_list.empty()) {
            return nullptr;
        } else {
            // Remove the thread from the queue.
            auto it = wait_list.begin();
            KThread* thread = std::addressof(*it);
            wait_list.erase(it);

            ASSERT(thread->GetState() == ThreadState::Waiting);

            // Mark the thread as no longer sleeping.
            thread->SetState(ThreadState::Runnable);
            thread->SetSleepingQueue(nullptr);

            return thread;
        }
    }

private:
    KernelCore& kernel;
    KThread::WaiterList wait_list{};
};

} // namespace Kernel
