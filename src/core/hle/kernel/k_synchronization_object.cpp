// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

void KSynchronizationObject::Finalize() {
    this->OnFinalizeSynchronizationObject();
    KAutoObject::Finalize();
}

ResultCode KSynchronizationObject::Wait(KernelCore& kernel_ctx, s32* out_index,
                                        KSynchronizationObject** objects, const s32 num_objects,
                                        s64 timeout) {
    // Allocate space on stack for thread nodes.
    std::vector<ThreadListNode> thread_nodes(num_objects);

    // Prepare for wait.
    KThread* thread = kernel_ctx.CurrentScheduler()->GetCurrentThread();

    {
        // Setup the scheduling lock and sleep.
        KScopedSchedulerLockAndSleep slp{kernel_ctx, thread, timeout};

        // Check if any of the objects are already signaled.
        for (auto i = 0; i < num_objects; ++i) {
            ASSERT(objects[i] != nullptr);

            if (objects[i]->IsSignaled()) {
                *out_index = i;
                slp.CancelSleep();
                return ResultSuccess;
            }
        }

        // Check if the timeout is zero.
        if (timeout == 0) {
            slp.CancelSleep();
            return ResultTimedOut;
        }

        // Check if the thread should terminate.
        if (thread->IsTerminationRequested()) {
            slp.CancelSleep();
            return ResultTerminationRequested;
        }

        // Check if waiting was canceled.
        if (thread->IsWaitCancelled()) {
            slp.CancelSleep();
            thread->ClearWaitCancelled();
            return ResultCancelled;
        }

        // Add the waiters.
        for (auto i = 0; i < num_objects; ++i) {
            thread_nodes[i].thread = thread;
            thread_nodes[i].next = nullptr;

            if (objects[i]->thread_list_tail == nullptr) {
                objects[i]->thread_list_head = std::addressof(thread_nodes[i]);
            } else {
                objects[i]->thread_list_tail->next = std::addressof(thread_nodes[i]);
            }

            objects[i]->thread_list_tail = std::addressof(thread_nodes[i]);
        }

        // For debugging only
        thread->SetWaitObjectsForDebugging({objects, static_cast<std::size_t>(num_objects)});

        // Mark the thread as waiting.
        thread->SetCancellable();
        thread->SetSyncedObject(nullptr, ResultTimedOut);
        thread->SetState(ThreadState::Waiting);
        thread->SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::Synchronization);
    }

    // The lock/sleep is done, so we should be able to get our result.

    // Thread is no longer cancellable.
    thread->ClearCancellable();

    // For debugging only
    thread->SetWaitObjectsForDebugging({});

    // Cancel the timer as needed.
    kernel_ctx.TimeManager().UnscheduleTimeEvent(thread);

    // Get the wait result.
    ResultCode wait_result{ResultSuccess};
    s32 sync_index = -1;
    {
        KScopedSchedulerLock lock(kernel_ctx);
        KSynchronizationObject* synced_obj;
        wait_result = thread->GetWaitResult(std::addressof(synced_obj));

        for (auto i = 0; i < num_objects; ++i) {
            // Unlink the object from the list.
            ThreadListNode* prev_ptr =
                reinterpret_cast<ThreadListNode*>(std::addressof(objects[i]->thread_list_head));
            ThreadListNode* prev_val = nullptr;
            ThreadListNode *prev, *tail_prev;

            do {
                prev = prev_ptr;
                prev_ptr = prev_ptr->next;
                tail_prev = prev_val;
                prev_val = prev_ptr;
            } while (prev_ptr != std::addressof(thread_nodes[i]));

            if (objects[i]->thread_list_tail == std::addressof(thread_nodes[i])) {
                objects[i]->thread_list_tail = tail_prev;
            }

            prev->next = thread_nodes[i].next;

            if (objects[i] == synced_obj) {
                sync_index = i;
            }
        }
    }

    // Set output.
    *out_index = sync_index;
    return wait_result;
}

KSynchronizationObject::KSynchronizationObject(KernelCore& kernel_)
    : KAutoObjectWithList{kernel_} {}

KSynchronizationObject::~KSynchronizationObject() = default;

void KSynchronizationObject::NotifyAvailable(ResultCode result) {
    KScopedSchedulerLock lock(kernel);

    // If we're not signaled, we've nothing to notify.
    if (!this->IsSignaled()) {
        return;
    }

    // Iterate over each thread.
    for (auto* cur_node = thread_list_head; cur_node != nullptr; cur_node = cur_node->next) {
        KThread* thread = cur_node->thread;
        if (thread->GetState() == ThreadState::Waiting) {
            thread->SetSyncedObject(this, result);
            thread->SetState(ThreadState::Runnable);
        }
    }
}

std::vector<KThread*> KSynchronizationObject::GetWaitingThreadsForDebugging() const {
    std::vector<KThread*> threads;

    // If debugging, dump the list of waiters.
    {
        KScopedSchedulerLock lock(kernel);
        for (auto* cur_node = thread_list_head; cur_node != nullptr; cur_node = cur_node->next) {
            threads.emplace_back(cur_node->thread);
        }
    }

    return threads;
}
} // namespace Kernel
