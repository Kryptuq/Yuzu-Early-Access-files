// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

void KLightLock::Lock() {
    const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer(kernel));
    const uintptr_t cur_thread_tag = (cur_thread | 1);

    while (true) {
        uintptr_t old_tag = tag.load(std::memory_order_relaxed);

        while (!tag.compare_exchange_weak(old_tag, (old_tag == 0) ? cur_thread : old_tag | 1,
                                          std::memory_order_acquire)) {
            if ((old_tag | 1) == cur_thread_tag) {
                return;
            }
        }

        if ((old_tag == 0) || ((old_tag | 1) == cur_thread_tag)) {
            break;
        }

        LockSlowPath(old_tag | 1, cur_thread);
    }
}

void KLightLock::Unlock() {
    const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer(kernel));
    uintptr_t expected = cur_thread;
    do {
        if (expected != cur_thread) {
            return UnlockSlowPath(cur_thread);
        }
    } while (!tag.compare_exchange_weak(expected, 0, std::memory_order_release));
}

void KLightLock::LockSlowPath(uintptr_t _owner, uintptr_t _cur_thread) {
    KThread* cur_thread = reinterpret_cast<KThread*>(_cur_thread);

    // Pend the current thread waiting on the owner thread.
    {
        KScopedSchedulerLock sl{kernel};

        // Ensure we actually have locking to do.
        if (tag.load(std::memory_order_relaxed) != _owner) {
            return;
        }

        // Add the current thread as a waiter on the owner.
        KThread* owner_thread = reinterpret_cast<KThread*>(_owner & ~1ULL);
        cur_thread->SetAddressKey(reinterpret_cast<uintptr_t>(std::addressof(tag)));
        owner_thread->AddWaiter(cur_thread);

        // Set thread states.
        if (cur_thread->GetState() == ThreadState::Runnable) {
            cur_thread->SetState(ThreadState::Waiting);
        } else {
            KScheduler::SetSchedulerUpdateNeeded(kernel);
        }

        if (owner_thread->IsSuspended()) {
            owner_thread->ContinueIfHasKernelWaiters();
        }
    }

    // We're no longer waiting on the lock owner.
    {
        KScopedSchedulerLock sl{kernel};
        KThread* owner_thread = cur_thread->GetLockOwner();
        if (owner_thread) {
            owner_thread->RemoveWaiter(cur_thread);
            KScheduler::SetSchedulerUpdateNeeded(kernel);
        }
    }
}

void KLightLock::UnlockSlowPath(uintptr_t _cur_thread) {
    KThread* owner_thread = reinterpret_cast<KThread*>(_cur_thread);

    // Unlock.
    {
        KScopedSchedulerLock sl{kernel};

        // Get the next owner.
        s32 num_waiters = 0;
        KThread* next_owner = owner_thread->RemoveWaiterByKey(
            std::addressof(num_waiters), reinterpret_cast<uintptr_t>(std::addressof(tag)));

        // Pass the lock to the next owner.
        uintptr_t next_tag = 0;
        if (next_owner) {
            next_tag = reinterpret_cast<uintptr_t>(next_owner);
            if (num_waiters > 1) {
                next_tag |= 0x1;
            }

            if (next_owner->GetState() == ThreadState::Waiting) {
                next_owner->SetState(ThreadState::Runnable);
            } else {
                KScheduler::SetSchedulerUpdateNeeded(kernel);
            }

            if (next_owner->IsSuspended()) {
                next_owner->ContinueIfHasKernelWaiters();
            }
        }

        // We may have unsuspended in the process of acquiring the lock, so we'll re-suspend now if
        // so.
        if (owner_thread->IsSuspended()) {
            owner_thread->TrySuspend();
        }

        // Write the new tag value.
        tag.store(next_tag);
    }
}

bool KLightLock::IsLockedByCurrentThread() const {
    return (tag | 1ULL) == (reinterpret_cast<uintptr_t>(GetCurrentThreadPointer(kernel)) | 1ULL);
}

} // namespace Kernel
