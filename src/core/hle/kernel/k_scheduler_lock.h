// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include "common/assert.h"
#include "core/hle/kernel/k_interrupt_manager.h"
#include "core/hle/kernel/k_spin_lock.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

class KernelCore;
class GlobalSchedulerContext;

template <typename SchedulerType>
class KAbstractSchedulerLock {
public:
    explicit KAbstractSchedulerLock(KernelCore& kernel) : m_kernel{kernel} {}

    bool IsLockedByCurrentThread() const {
        return m_owner_thread == GetCurrentThreadPointer(m_kernel);
    }

    void Lock() {
        if (this->IsLockedByCurrentThread()) {
            // If we already own the lock, the lock count should be > 0.
            // For debug, ensure this is true.
            ASSERT(m_lock_count > 0);
        } else {
            // Otherwise, we want to disable scheduling and acquire the spinlock.
            SchedulerType::DisableScheduling(m_kernel);
            m_spin_lock.Lock();

            ASSERT(m_lock_count == 0);
            ASSERT(m_owner_thread == nullptr);

            // Take ownership of the lock.
            m_owner_thread = GetCurrentThreadPointer(m_kernel);
        }

        // Increment the lock count.
        m_lock_count++;
    }

    void Unlock() {
        ASSERT(this->IsLockedByCurrentThread());
        ASSERT(m_lock_count > 0);

        // Release an instance of the lock.
        if ((--m_lock_count) == 0) {
            // Perform a memory barrier here.
            std::atomic_thread_fence(std::memory_order_seq_cst);

            // We're no longer going to hold the lock. Take note of what cores need scheduling.
            const u64 cores_needing_scheduling =
                SchedulerType::UpdateHighestPriorityThreads(m_kernel);

            // Note that we no longer hold the lock, and unlock the spinlock.
            m_owner_thread = nullptr;
            m_spin_lock.Unlock();

            // Enable scheduling, and perform a rescheduling operation.
            SchedulerType::EnableScheduling(m_kernel, cores_needing_scheduling);
        }
    }

private:
    friend class GlobalSchedulerContext;

    KernelCore& m_kernel;
    KAlignedSpinLock m_spin_lock{};
    s32 m_lock_count{};
    std::atomic<KThread*> m_owner_thread{};
};

} // namespace Kernel
