// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>

#include "core/arm/exclusive_monitor.h"
#include "core/core.h"
#include "core/hle/kernel/k_condition_variable.h"
#include "core/hle/kernel/k_linked_list.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_common.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel {

namespace {

bool ReadFromUser(Core::System& system, u32* out, VAddr address) {
    *out = system.Memory().Read32(address);
    return true;
}

bool WriteToUser(Core::System& system, VAddr address, const u32* p) {
    system.Memory().Write32(address, *p);
    return true;
}

bool UpdateLockAtomic(Core::System& system, u32* out, VAddr address, u32 if_zero,
                      u32 new_orr_mask) {
    auto& monitor = system.Monitor();
    const auto current_core = system.CurrentCoreIndex();

    // Load the value from the address.
    const auto expected = monitor.ExclusiveRead32(current_core, address);

    // Orr in the new mask.
    u32 value = expected | new_orr_mask;

    // If the value is zero, use the if_zero value, otherwise use the newly orr'd value.
    if (!expected) {
        value = if_zero;
    }

    // Try to store.
    if (!monitor.ExclusiveWrite32(current_core, address, value)) {
        // If we failed to store, try again.
        return UpdateLockAtomic(system, out, address, if_zero, new_orr_mask);
    }

    // We're done.
    *out = expected;
    return true;
}

} // namespace

KConditionVariable::KConditionVariable(Core::System& system_)
    : system{system_}, kernel{system.Kernel()} {}

KConditionVariable::~KConditionVariable() = default;

ResultCode KConditionVariable::SignalToAddress(VAddr addr) {
    KThread* owner_thread = kernel.CurrentScheduler()->GetCurrentThread();

    // Signal the address.
    {
        KScopedSchedulerLock sl(kernel);

        // Remove waiter thread.
        s32 num_waiters{};
        KThread* next_owner_thread =
            owner_thread->RemoveWaiterByKey(std::addressof(num_waiters), addr);

        // Determine the next tag.
        u32 next_value{};
        if (next_owner_thread) {
            next_value = next_owner_thread->GetAddressKeyValue();
            if (num_waiters > 1) {
                next_value |= Svc::HandleWaitMask;
            }

            next_owner_thread->SetSyncedObject(nullptr, ResultSuccess);
            next_owner_thread->Wakeup();
        }

        // Write the value to userspace.
        if (!WriteToUser(system, addr, std::addressof(next_value))) {
            if (next_owner_thread) {
                next_owner_thread->SetSyncedObject(nullptr, ResultInvalidCurrentMemory);
            }

            return ResultInvalidCurrentMemory;
        }
    }

    return ResultSuccess;
}

ResultCode KConditionVariable::WaitForAddress(Handle handle, VAddr addr, u32 value) {
    KThread* cur_thread = kernel.CurrentScheduler()->GetCurrentThread();

    // Wait for the address.
    {
        KScopedAutoObject<KThread> owner_thread;
        ASSERT(owner_thread.IsNull());
        {
            KScopedSchedulerLock sl(kernel);
            cur_thread->SetSyncedObject(nullptr, ResultSuccess);

            // Check if the thread should terminate.
            R_UNLESS(!cur_thread->IsTerminationRequested(), ResultTerminationRequested);

            {
                // Read the tag from userspace.
                u32 test_tag{};
                R_UNLESS(ReadFromUser(system, std::addressof(test_tag), addr),
                         ResultInvalidCurrentMemory);

                // If the tag isn't the handle (with wait mask), we're done.
                R_UNLESS(test_tag == (handle | Svc::HandleWaitMask), ResultSuccess);

                // Get the lock owner thread.
                owner_thread =
                    kernel.CurrentProcess()->GetHandleTable().GetObjectWithoutPseudoHandle<KThread>(
                        handle);
                R_UNLESS(owner_thread.IsNotNull(), ResultInvalidHandle);

                // Update the lock.
                cur_thread->SetAddressKey(addr, value);
                owner_thread->AddWaiter(cur_thread);
                cur_thread->SetState(ThreadState::Waiting);
                cur_thread->SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::ConditionVar);
                cur_thread->SetMutexWaitAddressForDebugging(addr);
            }
        }
        ASSERT(owner_thread.IsNotNull());
    }

    // Remove the thread as a waiter from the lock owner.
    {
        KScopedSchedulerLock sl(kernel);
        KThread* owner_thread = cur_thread->GetLockOwner();
        if (owner_thread != nullptr) {
            owner_thread->RemoveWaiter(cur_thread);
        }
    }

    // Get the wait result.
    KSynchronizationObject* dummy{};
    return cur_thread->GetWaitResult(std::addressof(dummy));
}

KThread* KConditionVariable::SignalImpl(KThread* thread) {
    // Check pre-conditions.
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Update the tag.
    VAddr address = thread->GetAddressKey();
    u32 own_tag = thread->GetAddressKeyValue();

    u32 prev_tag{};
    bool can_access{};
    {
        // TODO(bunnei): We should disable interrupts here via KScopedInterruptDisable.
        // TODO(bunnei): We should call CanAccessAtomic(..) here.
        can_access = true;
        if (can_access) {
            UpdateLockAtomic(system, std::addressof(prev_tag), address, own_tag,
                             Svc::HandleWaitMask);
        }
    }

    KThread* thread_to_close = nullptr;
    if (can_access) {
        if (prev_tag == Svc::InvalidHandle) {
            // If nobody held the lock previously, we're all good.
            thread->SetSyncedObject(nullptr, ResultSuccess);
            thread->Wakeup();
        } else {
            // Get the previous owner.
            KThread* owner_thread = kernel.CurrentProcess()
                                        ->GetHandleTable()
                                        .GetObjectWithoutPseudoHandle<KThread>(
                                            static_cast<Handle>(prev_tag & ~Svc::HandleWaitMask))
                                        .ReleasePointerUnsafe();

            if (owner_thread) {
                // Add the thread as a waiter on the owner.
                owner_thread->AddWaiter(thread);
                thread_to_close = owner_thread;
            } else {
                // The lock was tagged with a thread that doesn't exist.
                thread->SetSyncedObject(nullptr, ResultInvalidState);
                thread->Wakeup();
            }
        }
    } else {
        // If the address wasn't accessible, note so.
        thread->SetSyncedObject(nullptr, ResultInvalidCurrentMemory);
        thread->Wakeup();
    }

    return thread_to_close;
}

void KConditionVariable::Signal(u64 cv_key, s32 count) {
    // Prepare for signaling.
    constexpr int MaxThreads = 16;

    KLinkedList<KThread> thread_list{kernel};
    std::array<KThread*, MaxThreads> thread_array;
    s32 num_to_close{};

    // Perform signaling.
    s32 num_waiters{};
    {
        KScopedSchedulerLock sl(kernel);

        auto it = thread_tree.nfind_light({cv_key, -1});
        while ((it != thread_tree.end()) && (count <= 0 || num_waiters < count) &&
               (it->GetConditionVariableKey() == cv_key)) {
            KThread* target_thread = std::addressof(*it);

            if (KThread* thread = SignalImpl(target_thread); thread != nullptr) {
                if (num_to_close < MaxThreads) {
                    thread_array[num_to_close++] = thread;
                } else {
                    thread_list.push_back(*thread);
                }
            }

            it = thread_tree.erase(it);
            target_thread->ClearConditionVariable();
            ++num_waiters;
        }

        // If we have no waiters, clear the has waiter flag.
        if (it == thread_tree.end() || it->GetConditionVariableKey() != cv_key) {
            const u32 has_waiter_flag{};
            WriteToUser(system, cv_key, std::addressof(has_waiter_flag));
        }
    }

    // Close threads in the array.
    for (auto i = 0; i < num_to_close; ++i) {
        thread_array[i]->Close();
    }

    // Close threads in the list.
    for (auto it = thread_list.begin(); it != thread_list.end(); it = thread_list.erase(it)) {
        (*it).Close();
    }
}

ResultCode KConditionVariable::Wait(VAddr addr, u64 key, u32 value, s64 timeout) {
    // Prepare to wait.
    KThread* cur_thread = kernel.CurrentScheduler()->GetCurrentThread();

    {
        KScopedSchedulerLockAndSleep slp{kernel, cur_thread, timeout};

        // Set the synced object.
        cur_thread->SetSyncedObject(nullptr, ResultTimedOut);

        // Check that the thread isn't terminating.
        if (cur_thread->IsTerminationRequested()) {
            slp.CancelSleep();
            return ResultTerminationRequested;
        }

        // Update the value and process for the next owner.
        {
            // Remove waiter thread.
            s32 num_waiters{};
            KThread* next_owner_thread =
                cur_thread->RemoveWaiterByKey(std::addressof(num_waiters), addr);

            // Update for the next owner thread.
            u32 next_value{};
            if (next_owner_thread != nullptr) {
                // Get the next tag value.
                next_value = next_owner_thread->GetAddressKeyValue();
                if (num_waiters > 1) {
                    next_value |= Svc::HandleWaitMask;
                }

                // Wake up the next owner.
                next_owner_thread->SetSyncedObject(nullptr, ResultSuccess);
                next_owner_thread->Wakeup();
            }

            // Write to the cv key.
            {
                const u32 has_waiter_flag = 1;
                WriteToUser(system, key, std::addressof(has_waiter_flag));
                // TODO(bunnei): We should call DataMemoryBarrier(..) here.
            }

            // Write the value to userspace.
            if (!WriteToUser(system, addr, std::addressof(next_value))) {
                slp.CancelSleep();
                return ResultInvalidCurrentMemory;
            }
        }

        // Update condition variable tracking.
        {
            cur_thread->SetConditionVariable(std::addressof(thread_tree), addr, key, value);
            thread_tree.insert(*cur_thread);
        }

        // If the timeout is non-zero, set the thread as waiting.
        if (timeout != 0) {
            cur_thread->SetState(ThreadState::Waiting);
            cur_thread->SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::ConditionVar);
            cur_thread->SetMutexWaitAddressForDebugging(addr);
        }
    }

    // Cancel the timer wait.
    kernel.TimeManager().UnscheduleTimeEvent(cur_thread);

    // Remove from the condition variable.
    {
        KScopedSchedulerLock sl(kernel);

        if (KThread* owner = cur_thread->GetLockOwner(); owner != nullptr) {
            owner->RemoveWaiter(cur_thread);
        }

        if (cur_thread->IsWaitingForConditionVariable()) {
            thread_tree.erase(thread_tree.iterator_to(*cur_thread));
            cur_thread->ClearConditionVariable();
        }
    }

    // Get the result.
    KSynchronizationObject* dummy{};
    return cur_thread->GetWaitResult(std::addressof(dummy));
}

} // namespace Kernel
