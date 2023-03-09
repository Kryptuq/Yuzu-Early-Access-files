// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_hardware_timer.h"
#include "core/hle/kernel/k_thread_queue.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

void KThreadQueue::NotifyAvailable([[maybe_unused]] KThread* waiting_thread,
                                   [[maybe_unused]] KSynchronizationObject* signaled_object,
                                   [[maybe_unused]] Result wait_result) {}

void KThreadQueue::EndWait(KThread* waiting_thread, Result wait_result) {
    // Set the thread's wait result.
    waiting_thread->SetWaitResult(wait_result);

    // Set the thread as runnable.
    waiting_thread->SetState(ThreadState::Runnable);

    // Clear the thread's wait queue.
    waiting_thread->ClearWaitQueue();

    // Cancel the thread task.
    if (m_hardware_timer != nullptr) {
        m_hardware_timer->CancelTask(waiting_thread);
    }
}

void KThreadQueue::CancelWait(KThread* waiting_thread, Result wait_result, bool cancel_timer_task) {
    // Set the thread's wait result.
    waiting_thread->SetWaitResult(wait_result);

    // Set the thread as runnable.
    waiting_thread->SetState(ThreadState::Runnable);

    // Clear the thread's wait queue.
    waiting_thread->ClearWaitQueue();

    // Cancel the thread task.
    if (cancel_timer_task && m_hardware_timer != nullptr) {
        m_hardware_timer->CancelTask(waiting_thread);
    }
}

void KThreadQueueWithoutEndWait::EndWait([[maybe_unused]] KThread* waiting_thread,
                                         [[maybe_unused]] Result wait_result) {}

} // namespace Kernel
