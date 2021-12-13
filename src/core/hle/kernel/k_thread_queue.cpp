// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/k_thread_queue.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/time_manager.h"

namespace Kernel {

void KThreadQueue::NotifyAvailable([[maybe_unused]] KThread* waiting_thread,
                                   [[maybe_unused]] KSynchronizationObject* signaled_object,
                                   [[maybe_unused]] ResultCode wait_result) {}

void KThreadQueue::EndWait(KThread* waiting_thread, ResultCode wait_result) {
    // Set the thread's wait result.
    waiting_thread->SetWaitResult(wait_result);

    // Set the thread as runnable.
    waiting_thread->SetState(ThreadState::Runnable);

    // Clear the thread's wait queue.
    waiting_thread->ClearWaitQueue();

    // Cancel the thread task.
    kernel.TimeManager().UnscheduleTimeEvent(waiting_thread);
}

void KThreadQueue::CancelWait(KThread* waiting_thread, ResultCode wait_result,
                              bool cancel_timer_task) {
    // Set the thread's wait result.
    waiting_thread->SetWaitResult(wait_result);

    // Set the thread as runnable.
    waiting_thread->SetState(ThreadState::Runnable);

    // Clear the thread's wait queue.
    waiting_thread->ClearWaitQueue();

    // Cancel the thread task.
    if (cancel_timer_task) {
        kernel.TimeManager().UnscheduleTimeEvent(waiting_thread);
    }
}

void KThreadQueueWithoutEndWait::EndWait([[maybe_unused]] KThread* waiting_thread,
                                         [[maybe_unused]] ResultCode wait_result) {}

} // namespace Kernel
