// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_thread_queue.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

namespace {

class ThreadQueueImplForKLightLock final : public KThreadQueue {
public:
    explicit ThreadQueueImplForKLightLock(KernelCore& kernel_) : KThreadQueue(kernel_) {}

    virtual void CancelWait([[maybe_unused]] KThread* waiting_thread,
                            [[maybe_unused]] ResultCode wait_result,
                            [[maybe_unused]] bool cancel_timer_task) override {
        // Do nothing, waiting to acquire a light lock cannot be canceled.
    }
};

} // namespace

void KLightLock::Lock() {
    const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer(kernel));

    while (true) {
        uintptr_t old_tag = tag.load(std::memory_order_relaxed);

        while (!tag.compare_exchange_weak(old_tag, (old_tag == 0) ? cur_thread : (old_tag | 1),
                                          std::memory_order_acquire)) {
        }

        if (old_tag == 0 || this->LockSlowPath(old_tag | 1, cur_thread)) {
            break;
        }
    }
}

void KLightLock::Unlock() {
    const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer(kernel));

    uintptr_t expected = cur_thread;
    if (!tag.compare_exchange_strong(expected, 0, std::memory_order_release)) {
        this->UnlockSlowPath(cur_thread);
    }
}

bool KLightLock::LockSlowPath(uintptr_t _owner, uintptr_t _cur_thread) {
    KThread* cur_thread = reinterpret_cast<KThread*>(_cur_thread);
    ThreadQueueImplForKLightLock wait_queue(kernel);

    // Pend the current thread waiting on the owner thread.
    {
        KScopedSchedulerLock sl{kernel};

        // Ensure we actually have locking to do.
        if (tag.load(std::memory_order_relaxed) != _owner) {
            return false;
        }

        // Add the current thread as a waiter on the owner.
        KThread* owner_thread = reinterpret_cast<KThread*>(_owner & ~1ul);
        cur_thread->SetAddressKey(reinterpret_cast<uintptr_t>(std::addressof(tag)));
        owner_thread->AddWaiter(cur_thread);

        // Begin waiting to hold the lock.
        cur_thread->BeginWait(std::addressof(wait_queue));

        if (owner_thread->IsSuspended()) {
            owner_thread->ContinueIfHasKernelWaiters();
        }
    }

    return true;
}

void KLightLock::UnlockSlowPath(uintptr_t _cur_thread) {
    KThread* owner_thread = reinterpret_cast<KThread*>(_cur_thread);

    // Unlock.
    {
        KScopedSchedulerLock sl(kernel);

        // Get the next owner.
        s32 num_waiters;
        KThread* next_owner = owner_thread->RemoveWaiterByKey(
            std::addressof(num_waiters), reinterpret_cast<uintptr_t>(std::addressof(tag)));

        // Pass the lock to the next owner.
        uintptr_t next_tag = 0;
        if (next_owner != nullptr) {
            next_tag =
                reinterpret_cast<uintptr_t>(next_owner) | static_cast<uintptr_t>(num_waiters > 1);

            next_owner->EndWait(ResultSuccess);

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
        tag.store(next_tag, std::memory_order_release);
    }
}

bool KLightLock::IsLockedByCurrentThread() const {
    return (tag | 1ULL) == (reinterpret_cast<uintptr_t>(GetCurrentThreadPointer(kernel)) | 1ULL);
}

} // namespace Kernel
