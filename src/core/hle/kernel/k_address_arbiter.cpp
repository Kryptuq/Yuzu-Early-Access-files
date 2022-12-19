// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/arm/exclusive_monitor.h"
#include "core/core.h"
#include "core/hle/kernel/k_address_arbiter.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_thread_queue.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel {

KAddressArbiter::KAddressArbiter(Core::System& system_)
    : system{system_}, kernel{system.Kernel()} {}
KAddressArbiter::~KAddressArbiter() = default;

namespace {

bool ReadFromUser(Core::System& system, s32* out, VAddr address) {
    *out = system.Memory().Read32(address);
    return true;
}

bool DecrementIfLessThan(Core::System& system, s32* out, VAddr address, s32 value) {
    auto& monitor = system.Monitor();
    const auto current_core = system.Kernel().CurrentPhysicalCoreIndex();

    // TODO(bunnei): We should disable interrupts here via KScopedInterruptDisable.
    // TODO(bunnei): We should call CanAccessAtomic(..) here.

    // Load the value from the address.
    const s32 current_value = static_cast<s32>(monitor.ExclusiveRead32(current_core, address));

    // Compare it to the desired one.
    if (current_value < value) {
        // If less than, we want to try to decrement.
        const s32 decrement_value = current_value - 1;

        // Decrement and try to store.
        if (!monitor.ExclusiveWrite32(current_core, address, static_cast<u32>(decrement_value))) {
            // If we failed to store, try again.
            DecrementIfLessThan(system, out, address, value);
        }
    } else {
        // Otherwise, clear our exclusive hold and finish
        monitor.ClearExclusive(current_core);
    }

    // We're done.
    *out = current_value;
    return true;
}

bool UpdateIfEqual(Core::System& system, s32* out, VAddr address, s32 value, s32 new_value) {
    auto& monitor = system.Monitor();
    const auto current_core = system.Kernel().CurrentPhysicalCoreIndex();

    // TODO(bunnei): We should disable interrupts here via KScopedInterruptDisable.
    // TODO(bunnei): We should call CanAccessAtomic(..) here.

    // Load the value from the address.
    const s32 current_value = static_cast<s32>(monitor.ExclusiveRead32(current_core, address));

    // Compare it to the desired one.
    if (current_value == value) {
        // If equal, we want to try to write the new value.

        // Try to store.
        if (!monitor.ExclusiveWrite32(current_core, address, static_cast<u32>(new_value))) {
            // If we failed to store, try again.
            UpdateIfEqual(system, out, address, value, new_value);
        }
    } else {
        // Otherwise, clear our exclusive hold and finish.
        monitor.ClearExclusive(current_core);
    }

    // We're done.
    *out = current_value;
    return true;
}

class ThreadQueueImplForKAddressArbiter final : public KThreadQueue {
public:
    explicit ThreadQueueImplForKAddressArbiter(KernelCore& kernel_, KAddressArbiter::ThreadTree* t)
        : KThreadQueue(kernel_), m_tree(t) {}

    void CancelWait(KThread* waiting_thread, Result wait_result, bool cancel_timer_task) override {
        // If the thread is waiting on an address arbiter, remove it from the tree.
        if (waiting_thread->IsWaitingForAddressArbiter()) {
            m_tree->erase(m_tree->iterator_to(*waiting_thread));
            waiting_thread->ClearAddressArbiter();
        }

        // Invoke the base cancel wait handler.
        KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
    }

private:
    KAddressArbiter::ThreadTree* m_tree;
};

} // namespace

Result KAddressArbiter::Signal(VAddr addr, s32 count) {
    // Perform signaling.
    s32 num_waiters{};
    {
        KScopedSchedulerLock sl(kernel);

        auto it = thread_tree.nfind_key({addr, -1});
        while ((it != thread_tree.end()) && (count <= 0 || num_waiters < count) &&
               (it->GetAddressArbiterKey() == addr)) {
            // End the thread's wait.
            KThread* target_thread = std::addressof(*it);
            target_thread->EndWait(ResultSuccess);

            ASSERT(target_thread->IsWaitingForAddressArbiter());
            target_thread->ClearAddressArbiter();

            it = thread_tree.erase(it);
            ++num_waiters;
        }
    }
    return ResultSuccess;
}

Result KAddressArbiter::SignalAndIncrementIfEqual(VAddr addr, s32 value, s32 count) {
    // Perform signaling.
    s32 num_waiters{};
    {
        KScopedSchedulerLock sl(kernel);

        // Check the userspace value.
        s32 user_value{};
        if (!UpdateIfEqual(system, &user_value, addr, value, value + 1)) {
            LOG_ERROR(Kernel, "Invalid current memory!");
            return ResultInvalidCurrentMemory;
        }
        if (user_value != value) {
            return ResultInvalidState;
        }

        auto it = thread_tree.nfind_key({addr, -1});
        while ((it != thread_tree.end()) && (count <= 0 || num_waiters < count) &&
               (it->GetAddressArbiterKey() == addr)) {
            // End the thread's wait.
            KThread* target_thread = std::addressof(*it);
            target_thread->EndWait(ResultSuccess);

            ASSERT(target_thread->IsWaitingForAddressArbiter());
            target_thread->ClearAddressArbiter();

            it = thread_tree.erase(it);
            ++num_waiters;
        }
    }
    return ResultSuccess;
}

Result KAddressArbiter::SignalAndModifyByWaitingCountIfEqual(VAddr addr, s32 value, s32 count) {
    // Perform signaling.
    s32 num_waiters{};
    {
        [[maybe_unused]] const KScopedSchedulerLock sl(kernel);

        auto it = thread_tree.nfind_key({addr, -1});
        // Determine the updated value.
        s32 new_value{};
        if (count <= 0) {
            if (it != thread_tree.end() && it->GetAddressArbiterKey() == addr) {
                new_value = value - 2;
            } else {
                new_value = value + 1;
            }
        } else {
            if (it != thread_tree.end() && it->GetAddressArbiterKey() == addr) {
                auto tmp_it = it;
                s32 tmp_num_waiters{};
                while (++tmp_it != thread_tree.end() && tmp_it->GetAddressArbiterKey() == addr) {
                    if (tmp_num_waiters++ >= count) {
                        break;
                    }
                }

                if (tmp_num_waiters < count) {
                    new_value = value - 1;
                } else {
                    new_value = value;
                }
            } else {
                new_value = value + 1;
            }
        }

        // Check the userspace value.
        s32 user_value{};
        bool succeeded{};
        if (value != new_value) {
            succeeded = UpdateIfEqual(system, &user_value, addr, value, new_value);
        } else {
            succeeded = ReadFromUser(system, &user_value, addr);
        }

        if (!succeeded) {
            LOG_ERROR(Kernel, "Invalid current memory!");
            return ResultInvalidCurrentMemory;
        }
        if (user_value != value) {
            return ResultInvalidState;
        }

        while ((it != thread_tree.end()) && (count <= 0 || num_waiters < count) &&
               (it->GetAddressArbiterKey() == addr)) {
            // End the thread's wait.
            KThread* target_thread = std::addressof(*it);
            target_thread->EndWait(ResultSuccess);

            ASSERT(target_thread->IsWaitingForAddressArbiter());
            target_thread->ClearAddressArbiter();

            it = thread_tree.erase(it);
            ++num_waiters;
        }
    }
    return ResultSuccess;
}

Result KAddressArbiter::WaitIfLessThan(VAddr addr, s32 value, bool decrement, s64 timeout) {
    // Prepare to wait.
    KThread* cur_thread = GetCurrentThreadPointer(kernel);
    ThreadQueueImplForKAddressArbiter wait_queue(kernel, std::addressof(thread_tree));

    {
        KScopedSchedulerLockAndSleep slp{kernel, cur_thread, timeout};

        // Check that the thread isn't terminating.
        if (cur_thread->IsTerminationRequested()) {
            slp.CancelSleep();
            return ResultTerminationRequested;
        }

        // Read the value from userspace.
        s32 user_value{};
        bool succeeded{};
        if (decrement) {
            succeeded = DecrementIfLessThan(system, &user_value, addr, value);
        } else {
            succeeded = ReadFromUser(system, &user_value, addr);
        }

        if (!succeeded) {
            slp.CancelSleep();
            return ResultInvalidCurrentMemory;
        }

        // Check that the value is less than the specified one.
        if (user_value >= value) {
            slp.CancelSleep();
            return ResultInvalidState;
        }

        // Check that the timeout is non-zero.
        if (timeout == 0) {
            slp.CancelSleep();
            return ResultTimedOut;
        }

        // Set the arbiter.
        cur_thread->SetAddressArbiter(&thread_tree, addr);
        thread_tree.insert(*cur_thread);

        // Wait for the thread to finish.
        cur_thread->BeginWait(std::addressof(wait_queue));
        cur_thread->SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::Arbitration);
    }

    // Get the result.
    return cur_thread->GetWaitResult();
}

Result KAddressArbiter::WaitIfEqual(VAddr addr, s32 value, s64 timeout) {
    // Prepare to wait.
    KThread* cur_thread = GetCurrentThreadPointer(kernel);
    ThreadQueueImplForKAddressArbiter wait_queue(kernel, std::addressof(thread_tree));

    {
        KScopedSchedulerLockAndSleep slp{kernel, cur_thread, timeout};

        // Check that the thread isn't terminating.
        if (cur_thread->IsTerminationRequested()) {
            slp.CancelSleep();
            return ResultTerminationRequested;
        }

        // Read the value from userspace.
        s32 user_value{};
        if (!ReadFromUser(system, &user_value, addr)) {
            slp.CancelSleep();
            return ResultInvalidCurrentMemory;
        }

        // Check that the value is equal.
        if (value != user_value) {
            slp.CancelSleep();
            return ResultInvalidState;
        }

        // Check that the timeout is non-zero.
        if (timeout == 0) {
            slp.CancelSleep();
            return ResultTimedOut;
        }

        // Set the arbiter.
        cur_thread->SetAddressArbiter(&thread_tree, addr);
        thread_tree.insert(*cur_thread);

        // Wait for the thread to finish.
        cur_thread->BeginWait(std::addressof(wait_queue));
        cur_thread->SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::Arbitration);
    }

    // Get the result.
    return cur_thread->GetWaitResult();
}

} // namespace Kernel
