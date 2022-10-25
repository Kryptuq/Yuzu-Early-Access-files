// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <set>
#include <vector>

#include "common/common_types.h"
#include "core/hardware_properties.h"
#include "core/hle/kernel/k_priority_queue.h"
#include "core/hle/kernel/k_scheduler_lock.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/svc_types.h"

namespace Kernel {

class KernelCore;
class SchedulerLock;

using KSchedulerPriorityQueue =
    KPriorityQueue<KThread, Core::Hardware::NUM_CPU_CORES, Svc::LowestThreadPriority,
                   Svc::HighestThreadPriority>;

static constexpr s32 HighestCoreMigrationAllowedPriority = 2;
static_assert(Svc::LowestThreadPriority >= HighestCoreMigrationAllowedPriority);
static_assert(Svc::HighestThreadPriority <= HighestCoreMigrationAllowedPriority);

class GlobalSchedulerContext final {
    friend class KScheduler;

public:
    using LockType = KAbstractSchedulerLock<KScheduler>;

    explicit GlobalSchedulerContext(KernelCore& kernel_);
    ~GlobalSchedulerContext();

    /// Adds a new thread to the scheduler
    void AddThread(KThread* thread);

    /// Removes a thread from the scheduler
    void RemoveThread(KThread* thread);

    /// Returns a list of all threads managed by the scheduler
    [[nodiscard]] const std::vector<KThread*>& GetThreadList() const {
        return thread_list;
    }

    /**
     * Rotates the scheduling queues of threads at a preemption priority and then does
     * some core rebalancing. Preemption priorities can be found in the array
     * 'preemption_priorities'.
     *
     * @note This operation happens every 10ms.
     */
    void PreemptThreads();

    /// Returns true if the global scheduler lock is acquired
    bool IsLocked() const;

    void UnregisterDummyThreadForWakeup(KThread* thread);
    void RegisterDummyThreadForWakeup(KThread* thread);
    void WakeupWaitingDummyThreads();

    [[nodiscard]] LockType& SchedulerLock() {
        return scheduler_lock;
    }

    [[nodiscard]] const LockType& SchedulerLock() const {
        return scheduler_lock;
    }

private:
    friend class KScopedSchedulerLock;
    friend class KScopedSchedulerLockAndSleep;

    KernelCore& kernel;

    std::atomic_bool scheduler_update_needed{};
    KSchedulerPriorityQueue priority_queue;
    LockType scheduler_lock;

    /// Lists dummy threads pending wakeup on lock release
    std::set<KThread*> woken_dummy_threads;

    /// Lists all thread ids that aren't deleted/etc.
    std::vector<KThread*> thread_list;
    std::mutex global_list_guard;
};

} // namespace Kernel
