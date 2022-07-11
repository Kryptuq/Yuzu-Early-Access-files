// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>

#include "common/common_types.h"
#include "core/hle/kernel/global_scheduler_context.h"
#include "core/hle/kernel/k_priority_queue.h"
#include "core/hle/kernel/k_scheduler_lock.h"
#include "core/hle/kernel/k_scoped_lock.h"
#include "core/hle/kernel/k_spin_lock.h"
#include "core/hle/kernel/k_thread.h"

namespace Common {
class Fiber;
}

namespace Core {
class System;
}

namespace Kernel {

class KernelCore;
class KInterruptTaskManager;
class KProcess;
class KThread;
class KScopedDisableDispatch;
class KScopedSchedulerLock;
class KScopedSchedulerLockAndSleep;

class KScheduler final {
public:
    YUZU_NON_COPYABLE(KScheduler);
    YUZU_NON_MOVEABLE(KScheduler);

    using LockType = KAbstractSchedulerLock<KScheduler>;

    explicit KScheduler(KernelCore& kernel);
    ~KScheduler();

    void Initialize(KThread* main_thread, KThread* idle_thread, s32 core_id);
    void Activate();
    void OnThreadStart();
    void Unload(KThread* thread);
    void Reload(KThread* thread);

    void SetInterruptTaskRunnable();
    void RequestScheduleOnInterrupt();
    void PreemptSingleCore();

    u64 GetIdleCount() {
        return m_state.idle_count;
    }

    KThread* GetIdleThread() const {
        return m_idle_thread;
    }

    bool IsIdle() const {
        return m_current_thread.load() == m_idle_thread;
    }

    KThread* GetPreviousThread() const {
        return m_state.prev_thread;
    }

    KThread* GetSchedulerCurrentThread() const {
        return m_current_thread.load();
    }

    s64 GetLastContextSwitchTime() const {
        return m_last_context_switch_time;
    }

    // Static public API.
    static bool CanSchedule(KernelCore& kernel) {
        return GetCurrentThread(kernel).GetDisableDispatchCount() == 0;
    }
    static bool IsSchedulerLockedByCurrentThread(KernelCore& kernel) {
        return kernel.GlobalSchedulerContext().scheduler_lock.IsLockedByCurrentThread();
    }

    static bool IsSchedulerUpdateNeeded(KernelCore& kernel) {
        return kernel.GlobalSchedulerContext().scheduler_update_needed;
    }
    static void SetSchedulerUpdateNeeded(KernelCore& kernel) {
        kernel.GlobalSchedulerContext().scheduler_update_needed = true;
    }
    static void ClearSchedulerUpdateNeeded(KernelCore& kernel) {
        kernel.GlobalSchedulerContext().scheduler_update_needed = false;
    }

    static void DisableScheduling(KernelCore& kernel);
    static void EnableScheduling(KernelCore& kernel, u64 cores_needing_scheduling);

    static u64 UpdateHighestPriorityThreads(KernelCore& kernel);

    static void ClearPreviousThread(KernelCore& kernel, KThread* thread);

    static void OnThreadStateChanged(KernelCore& kernel, KThread* thread, ThreadState old_state);
    static void OnThreadPriorityChanged(KernelCore& kernel, KThread* thread, s32 old_priority);
    static void OnThreadAffinityMaskChanged(KernelCore& kernel, KThread* thread,
                                            const KAffinityMask& old_affinity, s32 old_core);

    static void RotateScheduledQueue(KernelCore& kernel, s32 core_id, s32 priority);
    static void RescheduleCores(KernelCore& kernel, u64 cores_needing_scheduling);

    static void YieldWithoutCoreMigration(KernelCore& kernel);
    static void YieldWithCoreMigration(KernelCore& kernel);
    static void YieldToAnyThread(KernelCore& kernel);

private:
    // Static private API.
    static KSchedulerPriorityQueue& GetPriorityQueue(KernelCore& kernel) {
        return kernel.GlobalSchedulerContext().priority_queue;
    }
    static u64 UpdateHighestPriorityThreadsImpl(KernelCore& kernel);

    static void RescheduleCurrentHLEThread(KernelCore& kernel);

    // Instanced private API.
    void ScheduleImpl();
    void ScheduleImplFiber();
    void SwitchThread(KThread* next_thread);

    void Schedule();
    void ScheduleOnInterrupt();

    void RescheduleOtherCores(u64 cores_needing_scheduling);
    void RescheduleCurrentCore();
    void RescheduleCurrentCoreImpl();

    u64 UpdateHighestPriorityThread(KThread* thread);

private:
    friend class KScopedDisableDispatch;

    struct SchedulingState {
        std::atomic<bool> needs_scheduling{false};
        bool interrupt_task_runnable{false};
        bool should_count_idle{false};
        u64 idle_count{0};
        KThread* highest_priority_thread{nullptr};
        void* idle_thread_stack{nullptr};
        std::atomic<KThread*> prev_thread{nullptr};
        KInterruptTaskManager* interrupt_task_manager{nullptr};
    };

    KernelCore& kernel;
    SchedulingState m_state;
    bool m_is_active{false};
    s32 m_core_id{0};
    s64 m_last_context_switch_time{0};
    KThread* m_idle_thread{nullptr};
    std::atomic<KThread*> m_current_thread{nullptr};

    std::shared_ptr<Common::Fiber> m_switch_fiber{};
    KThread* m_switch_cur_thread{};
    KThread* m_switch_highest_priority_thread{};
    bool m_switch_from_schedule{};
};

class KScopedSchedulerLock : public KScopedLock<KScheduler::LockType> {
public:
    explicit KScopedSchedulerLock(KernelCore& kernel)
        : KScopedLock(kernel.GlobalSchedulerContext().scheduler_lock) {}
    ~KScopedSchedulerLock() = default;
};

} // namespace Kernel
