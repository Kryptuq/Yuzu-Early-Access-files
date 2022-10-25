// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>

#include "common/assert.h"
#include "common/bit_util.h"
#include "common/fiber.h"
#include "common/logging/log.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu_manager.h"
#include "core/hle/kernel/k_interrupt_manager.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

static void IncrementScheduledCount(Kernel::KThread* thread) {
    if (auto process = thread->GetOwnerProcess(); process) {
        process->IncrementScheduledCount();
    }
}

KScheduler::KScheduler(KernelCore& kernel_) : kernel{kernel_} {
    m_switch_fiber = std::make_shared<Common::Fiber>([this] {
        while (true) {
            ScheduleImplFiber();
        }
    });

    m_state.needs_scheduling = true;
}

KScheduler::~KScheduler() = default;

void KScheduler::SetInterruptTaskRunnable() {
    m_state.interrupt_task_runnable = true;
    m_state.needs_scheduling = true;
}

void KScheduler::RequestScheduleOnInterrupt() {
    m_state.needs_scheduling = true;

    if (CanSchedule(kernel)) {
        ScheduleOnInterrupt();
    }
}

void KScheduler::DisableScheduling(KernelCore& kernel) {
    ASSERT(GetCurrentThread(kernel).GetDisableDispatchCount() >= 0);
    GetCurrentThread(kernel).DisableDispatch();
}

void KScheduler::EnableScheduling(KernelCore& kernel, u64 cores_needing_scheduling) {
    ASSERT(GetCurrentThread(kernel).GetDisableDispatchCount() >= 1);

    auto* scheduler{kernel.CurrentScheduler()};

    if (!scheduler || kernel.IsPhantomModeForSingleCore()) {
        KScheduler::RescheduleCores(kernel, cores_needing_scheduling);
        KScheduler::RescheduleCurrentHLEThread(kernel);
        return;
    }

    scheduler->RescheduleOtherCores(cores_needing_scheduling);

    if (GetCurrentThread(kernel).GetDisableDispatchCount() > 1) {
        GetCurrentThread(kernel).EnableDispatch();
    } else {
        scheduler->RescheduleCurrentCore();
    }
}

void KScheduler::RescheduleCurrentHLEThread(KernelCore& kernel) {
    // HACK: we cannot schedule from this thread, it is not a core thread
    ASSERT(GetCurrentThread(kernel).GetDisableDispatchCount() == 1);

    // Ensure dummy threads that are waiting block.
    GetCurrentThread(kernel).DummyThreadBeginWait();

    ASSERT(GetCurrentThread(kernel).GetState() != ThreadState::Waiting);
    GetCurrentThread(kernel).EnableDispatch();
}

u64 KScheduler::UpdateHighestPriorityThreads(KernelCore& kernel) {
    if (IsSchedulerUpdateNeeded(kernel)) {
        return UpdateHighestPriorityThreadsImpl(kernel);
    } else {
        return 0;
    }
}

void KScheduler::Schedule() {
    ASSERT(GetCurrentThread(kernel).GetDisableDispatchCount() == 1);
    ASSERT(m_core_id == GetCurrentCoreId(kernel));

    ScheduleImpl();
}

void KScheduler::ScheduleOnInterrupt() {
    GetCurrentThread(kernel).DisableDispatch();
    Schedule();
    GetCurrentThread(kernel).EnableDispatch();
}

void KScheduler::PreemptSingleCore() {
    GetCurrentThread(kernel).DisableDispatch();

    auto* thread = GetCurrentThreadPointer(kernel);
    auto& previous_scheduler = kernel.Scheduler(thread->GetCurrentCore());
    previous_scheduler.Unload(thread);

    Common::Fiber::YieldTo(thread->GetHostContext(), *m_switch_fiber);

    GetCurrentThread(kernel).EnableDispatch();
}

void KScheduler::RescheduleCurrentCore() {
    ASSERT(!kernel.IsPhantomModeForSingleCore());
    ASSERT(GetCurrentThread(kernel).GetDisableDispatchCount() == 1);

    GetCurrentThread(kernel).EnableDispatch();

    if (m_state.needs_scheduling.load()) {
        // Disable interrupts, and then check again if rescheduling is needed.
        // KScopedInterruptDisable intr_disable;

        kernel.CurrentScheduler()->RescheduleCurrentCoreImpl();
    }
}

void KScheduler::RescheduleCurrentCoreImpl() {
    // Check that scheduling is needed.
    if (m_state.needs_scheduling.load()) [[likely]] {
        GetCurrentThread(kernel).DisableDispatch();
        Schedule();
        GetCurrentThread(kernel).EnableDispatch();
    }
}

void KScheduler::Initialize(KThread* main_thread, KThread* idle_thread, s32 core_id) {
    // Set core ID/idle thread/interrupt task manager.
    m_core_id = core_id;
    m_idle_thread = idle_thread;
    // m_state.idle_thread_stack = m_idle_thread->GetStackTop();
    // m_state.interrupt_task_manager = &kernel.GetInterruptTaskManager();

    // Insert the main thread into the priority queue.
    // {
    //     KScopedSchedulerLock lk{kernel};
    //     GetPriorityQueue(kernel).PushBack(GetCurrentThreadPointer(kernel));
    //     SetSchedulerUpdateNeeded(kernel);
    // }

    // Bind interrupt handler.
    // kernel.GetInterruptManager().BindHandler(
    //     GetSchedulerInterruptHandler(kernel), KInterruptName::Scheduler, m_core_id,
    //     KInterruptController::PriorityLevel::Scheduler, false, false);

    // Set the current thread.
    m_current_thread = main_thread;
}

void KScheduler::Activate() {
    ASSERT(GetCurrentThread(kernel).GetDisableDispatchCount() == 1);

    // m_state.should_count_idle = KTargetSystem::IsDebugMode();
    m_is_active = true;
    RescheduleCurrentCore();
}

void KScheduler::OnThreadStart() {
    GetCurrentThread(kernel).EnableDispatch();
}

u64 KScheduler::UpdateHighestPriorityThread(KThread* highest_thread) {
    if (KThread* prev_highest_thread = m_state.highest_priority_thread;
        prev_highest_thread != highest_thread) [[likely]] {
        if (prev_highest_thread != nullptr) [[likely]] {
            IncrementScheduledCount(prev_highest_thread);
            prev_highest_thread->SetLastScheduledTick(kernel.System().CoreTiming().GetCPUTicks());
        }
        if (m_state.should_count_idle) {
            if (highest_thread != nullptr) [[likely]] {
                if (KProcess* process = highest_thread->GetOwnerProcess(); process != nullptr) {
                    process->SetRunningThread(m_core_id, highest_thread, m_state.idle_count);
                }
            } else {
                m_state.idle_count++;
            }
        }

        m_state.highest_priority_thread = highest_thread;
        m_state.needs_scheduling = true;
        return (1ULL << m_core_id);
    } else {
        return 0;
    }
}

u64 KScheduler::UpdateHighestPriorityThreadsImpl(KernelCore& kernel) {
    ASSERT(IsSchedulerLockedByCurrentThread(kernel));

    // Clear that we need to update.
    ClearSchedulerUpdateNeeded(kernel);

    u64 cores_needing_scheduling = 0, idle_cores = 0;
    KThread* top_threads[Core::Hardware::NUM_CPU_CORES];
    auto& priority_queue = GetPriorityQueue(kernel);

    // We want to go over all cores, finding the highest priority thread and determining if
    // scheduling is needed for that core.
    for (size_t core_id = 0; core_id < Core::Hardware::NUM_CPU_CORES; core_id++) {
        KThread* top_thread = priority_queue.GetScheduledFront(static_cast<s32>(core_id));
        if (top_thread != nullptr) {
            // We need to check if the thread's process has a pinned thread.
            if (KProcess* parent = top_thread->GetOwnerProcess()) {
                // Check that there's a pinned thread other than the current top thread.
                if (KThread* pinned = parent->GetPinnedThread(static_cast<s32>(core_id));
                    pinned != nullptr && pinned != top_thread) {
                    // We need to prefer threads with kernel waiters to the pinned thread.
                    if (top_thread->GetNumKernelWaiters() ==
                        0 /* && top_thread != parent->GetExceptionThread() */) {
                        // If the pinned thread is runnable, use it.
                        if (pinned->GetRawState() == ThreadState::Runnable) {
                            top_thread = pinned;
                        } else {
                            top_thread = nullptr;
                        }
                    }
                }
            }
        } else {
            idle_cores |= (1ULL << core_id);
        }

        top_threads[core_id] = top_thread;
        cores_needing_scheduling |=
            kernel.Scheduler(core_id).UpdateHighestPriorityThread(top_threads[core_id]);
    }

    // Idle cores are bad. We're going to try to migrate threads to each idle core in turn.
    while (idle_cores != 0) {
        const s32 core_id = static_cast<s32>(std::countr_zero(idle_cores));

        if (KThread* suggested = priority_queue.GetSuggestedFront(core_id); suggested != nullptr) {
            s32 migration_candidates[Core::Hardware::NUM_CPU_CORES];
            size_t num_candidates = 0;

            // While we have a suggested thread, try to migrate it!
            while (suggested != nullptr) {
                // Check if the suggested thread is the top thread on its core.
                const s32 suggested_core = suggested->GetActiveCore();
                if (KThread* top_thread =
                        (suggested_core >= 0) ? top_threads[suggested_core] : nullptr;
                    top_thread != suggested) {
                    // Make sure we're not dealing with threads too high priority for migration.
                    if (top_thread != nullptr &&
                        top_thread->GetPriority() < HighestCoreMigrationAllowedPriority) {
                        break;
                    }

                    // The suggested thread isn't bound to its core, so we can migrate it!
                    suggested->SetActiveCore(core_id);
                    priority_queue.ChangeCore(suggested_core, suggested);
                    top_threads[core_id] = suggested;
                    cores_needing_scheduling |=
                        kernel.Scheduler(core_id).UpdateHighestPriorityThread(top_threads[core_id]);
                    break;
                }

                // Note this core as a candidate for migration.
                ASSERT(num_candidates < Core::Hardware::NUM_CPU_CORES);
                migration_candidates[num_candidates++] = suggested_core;
                suggested = priority_queue.GetSuggestedNext(core_id, suggested);
            }

            // If suggested is nullptr, we failed to migrate a specific thread. So let's try all our
            // candidate cores' top threads.
            if (suggested == nullptr) {
                for (size_t i = 0; i < num_candidates; i++) {
                    // Check if there's some other thread that can run on the candidate core.
                    const s32 candidate_core = migration_candidates[i];
                    suggested = top_threads[candidate_core];
                    if (KThread* next_on_candidate_core =
                            priority_queue.GetScheduledNext(candidate_core, suggested);
                        next_on_candidate_core != nullptr) {
                        // The candidate core can run some other thread! We'll migrate its current
                        // top thread to us.
                        top_threads[candidate_core] = next_on_candidate_core;
                        cores_needing_scheduling |=
                            kernel.Scheduler(candidate_core)
                                .UpdateHighestPriorityThread(top_threads[candidate_core]);

                        // Perform the migration.
                        suggested->SetActiveCore(core_id);
                        priority_queue.ChangeCore(candidate_core, suggested);
                        top_threads[core_id] = suggested;
                        cores_needing_scheduling |=
                            kernel.Scheduler(core_id).UpdateHighestPriorityThread(
                                top_threads[core_id]);
                        break;
                    }
                }
            }
        }

        idle_cores &= ~(1ULL << core_id);
    }

    // HACK: any waiting dummy threads can wake up now.
    kernel.GlobalSchedulerContext().WakeupWaitingDummyThreads();

    // HACK: if we are a dummy thread, and we need to go sleep, indicate
    // that for when the lock is released.
    KThread* const cur_thread = GetCurrentThreadPointer(kernel);
    if (cur_thread->IsDummyThread() && cur_thread->GetState() != ThreadState::Runnable) {
        cur_thread->RequestDummyThreadWait();
    }

    return cores_needing_scheduling;
}

void KScheduler::SwitchThread(KThread* next_thread) {
    KProcess* const cur_process = kernel.CurrentProcess();
    KThread* const cur_thread = GetCurrentThreadPointer(kernel);

    // We never want to schedule a null thread, so use the idle thread if we don't have a next.
    if (next_thread == nullptr) {
        next_thread = m_idle_thread;
    }

    if (next_thread->GetCurrentCore() != m_core_id) {
        next_thread->SetCurrentCore(m_core_id);
    }

    // If we're not actually switching thread, there's nothing to do.
    if (next_thread == cur_thread) {
        return;
    }

    // Next thread is now known not to be nullptr, and must not be dispatchable.
    ASSERT(next_thread->GetDisableDispatchCount() == 1);
    ASSERT(!next_thread->IsDummyThread());

    // Update the CPU time tracking variables.
    const s64 prev_tick = m_last_context_switch_time;
    const s64 cur_tick = kernel.System().CoreTiming().GetCPUTicks();
    const s64 tick_diff = cur_tick - prev_tick;
    cur_thread->AddCpuTime(m_core_id, tick_diff);
    if (cur_process != nullptr) {
        cur_process->UpdateCPUTimeTicks(tick_diff);
    }
    m_last_context_switch_time = cur_tick;

    // Update our previous thread.
    if (cur_process != nullptr) {
        if (!cur_thread->IsTerminationRequested() && cur_thread->GetActiveCore() == m_core_id)
            [[likely]] {
            m_state.prev_thread = cur_thread;
        } else {
            m_state.prev_thread = nullptr;
        }
    }

    // Switch the current process, if we're switching processes.
    // if (KProcess *next_process = next_thread->GetOwnerProcess(); next_process != cur_process) {
    //     KProcess::Switch(cur_process, next_process);
    // }

    // Set the new thread.
    SetCurrentThread(kernel, next_thread);
    m_current_thread = next_thread;

    // Set the new Thread Local region.
    // cpu::SwitchThreadLocalRegion(GetInteger(next_thread->GetThreadLocalRegionAddress()));
}

void KScheduler::ScheduleImpl() {
    // First, clear the needs scheduling bool.
    m_state.needs_scheduling.store(false, std::memory_order_seq_cst);

    // Load the appropriate thread pointers for scheduling.
    KThread* const cur_thread{GetCurrentThreadPointer(kernel)};
    KThread* highest_priority_thread{m_state.highest_priority_thread};

    // Check whether there are runnable interrupt tasks.
    if (m_state.interrupt_task_runnable) {
        // The interrupt task is runnable.
        // We want to switch to the interrupt task/idle thread.
        highest_priority_thread = nullptr;
    }

    // If there aren't, we want to check if the highest priority thread is the same as the current
    // thread.
    if (highest_priority_thread == cur_thread) {
        // If they're the same, then we can just return.
        return;
    }

    // The highest priority thread is not the same as the current thread.
    // Jump to the switcher and continue executing from there.
    m_switch_cur_thread = cur_thread;
    m_switch_highest_priority_thread = highest_priority_thread;
    m_switch_from_schedule = true;
    Common::Fiber::YieldTo(cur_thread->host_context, *m_switch_fiber);

    // Returning from ScheduleImpl occurs after this thread has been scheduled again.
}

void KScheduler::ScheduleImplFiber() {
    KThread* const cur_thread{m_switch_cur_thread};
    KThread* highest_priority_thread{m_switch_highest_priority_thread};

    // If we're not coming from scheduling (i.e., we came from SC preemption),
    // we should restart the scheduling loop directly. Not accurate to HOS.
    if (!m_switch_from_schedule) {
        goto retry;
    }

    // Mark that we are not coming from scheduling anymore.
    m_switch_from_schedule = false;

    // Save the original thread context.
    Unload(cur_thread);

    // The current thread's context has been entirely taken care of.
    // Now we want to loop until we successfully switch the thread context.
    while (true) {
        // We're starting to try to do the context switch.
        // Check if the highest priority thread is null.
        if (!highest_priority_thread) {
            // The next thread is nullptr!

            // Switch to the idle thread. Note: HOS treats idling as a special case for
            // performance. This is not *required* for yuzu's purposes, and for singlecore
            // compatibility, we can just move the logic that would go here into the execution
            // of the idle thread. If we ever remove singlecore, we should implement this
            // accurately to HOS.
            highest_priority_thread = m_idle_thread;
        }

        // We want to try to lock the highest priority thread's context.
        // Try to take it.
        while (!highest_priority_thread->context_guard.try_lock()) {
            // The highest priority thread's context is already locked.
            // Check if we need scheduling. If we don't, we can retry directly.
            if (m_state.needs_scheduling.load(std::memory_order_seq_cst)) {
                // If we do, another core is interfering, and we must start again.
                goto retry;
            }
        }

        // It's time to switch the thread.
        // Switch to the highest priority thread.
        SwitchThread(highest_priority_thread);

        // Check if we need scheduling. If we do, then we can't complete the switch and should
        // retry.
        if (m_state.needs_scheduling.load(std::memory_order_seq_cst)) {
            // Our switch failed.
            // We should unlock the thread context, and then retry.
            highest_priority_thread->context_guard.unlock();
            goto retry;
        } else {
            break;
        }

    retry:

        // We failed to successfully do the context switch, and need to retry.
        // Clear needs_scheduling.
        m_state.needs_scheduling.store(false, std::memory_order_seq_cst);

        // Refresh the highest priority thread.
        highest_priority_thread = m_state.highest_priority_thread;
    }

    // Reload the guest thread context.
    Reload(highest_priority_thread);

    // Reload the host thread.
    Common::Fiber::YieldTo(m_switch_fiber, *highest_priority_thread->host_context);
}

void KScheduler::Unload(KThread* thread) {
    auto& cpu_core = kernel.System().ArmInterface(m_core_id);
    cpu_core.SaveContext(thread->GetContext32());
    cpu_core.SaveContext(thread->GetContext64());
    // Save the TPIDR_EL0 system register in case it was modified.
    thread->SetTPIDR_EL0(cpu_core.GetTPIDR_EL0());
    cpu_core.ClearExclusiveState();

    // Check if the thread is terminated by checking the DPC flags.
    if ((thread->GetStackParameters().dpc_flags & static_cast<u32>(DpcFlag::Terminated)) == 0) {
        // The thread isn't terminated, so we want to unlock it.
        thread->context_guard.unlock();
    }
}

void KScheduler::Reload(KThread* thread) {
    auto& cpu_core = kernel.System().ArmInterface(m_core_id);
    cpu_core.LoadContext(thread->GetContext32());
    cpu_core.LoadContext(thread->GetContext64());
    cpu_core.SetTlsAddress(thread->GetTLSAddress());
    cpu_core.SetTPIDR_EL0(thread->GetTPIDR_EL0());
    cpu_core.LoadWatchpointArray(thread->GetOwnerProcess()->GetWatchpoints());
    cpu_core.ClearExclusiveState();
}

void KScheduler::ClearPreviousThread(KernelCore& kernel, KThread* thread) {
    ASSERT(IsSchedulerLockedByCurrentThread(kernel));
    for (size_t i = 0; i < Core::Hardware::NUM_CPU_CORES; ++i) {
        // Get an atomic reference to the core scheduler's previous thread.
        auto& prev_thread{kernel.Scheduler(i).m_state.prev_thread};

        // Atomically clear the previous thread if it's our target.
        KThread* compare = thread;
        prev_thread.compare_exchange_strong(compare, nullptr, std::memory_order_seq_cst);
    }
}

void KScheduler::OnThreadStateChanged(KernelCore& kernel, KThread* thread, ThreadState old_state) {
    ASSERT(IsSchedulerLockedByCurrentThread(kernel));

    // Check if the state has changed, because if it hasn't there's nothing to do.
    const ThreadState cur_state = thread->GetRawState();
    if (cur_state == old_state) {
        return;
    }

    // Update the priority queues.
    if (old_state == ThreadState::Runnable) {
        // If we were previously runnable, then we're not runnable now, and we should remove.
        GetPriorityQueue(kernel).Remove(thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);

        if (thread->IsDummyThread()) {
            // HACK: if this is a dummy thread, it should no longer wake up when the
            // scheduler lock is released.
            kernel.GlobalSchedulerContext().UnregisterDummyThreadForWakeup(thread);
        }
    } else if (cur_state == ThreadState::Runnable) {
        // If we're now runnable, then we weren't previously, and we should add.
        GetPriorityQueue(kernel).PushBack(thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);

        if (thread->IsDummyThread()) {
            // HACK: if this is a dummy thread, it should wake up when the scheduler
            // lock is released.
            kernel.GlobalSchedulerContext().RegisterDummyThreadForWakeup(thread);
        }
    }
}

void KScheduler::OnThreadPriorityChanged(KernelCore& kernel, KThread* thread, s32 old_priority) {
    ASSERT(IsSchedulerLockedByCurrentThread(kernel));

    // If the thread is runnable, we want to change its priority in the queue.
    if (thread->GetRawState() == ThreadState::Runnable) {
        GetPriorityQueue(kernel).ChangePriority(old_priority,
                                                thread == GetCurrentThreadPointer(kernel), thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);
    }
}

void KScheduler::OnThreadAffinityMaskChanged(KernelCore& kernel, KThread* thread,
                                             const KAffinityMask& old_affinity, s32 old_core) {
    ASSERT(IsSchedulerLockedByCurrentThread(kernel));

    // If the thread is runnable, we want to change its affinity in the queue.
    if (thread->GetRawState() == ThreadState::Runnable) {
        GetPriorityQueue(kernel).ChangeAffinityMask(old_core, old_affinity, thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);
    }
}

void KScheduler::RotateScheduledQueue(KernelCore& kernel, s32 core_id, s32 priority) {
    ASSERT(IsSchedulerLockedByCurrentThread(kernel));

    // Get a reference to the priority queue.
    auto& priority_queue = GetPriorityQueue(kernel);

    // Rotate the front of the queue to the end.
    KThread* top_thread = priority_queue.GetScheduledFront(core_id, priority);
    KThread* next_thread = nullptr;
    if (top_thread != nullptr) {
        next_thread = priority_queue.MoveToScheduledBack(top_thread);
        if (next_thread != top_thread) {
            IncrementScheduledCount(top_thread);
            IncrementScheduledCount(next_thread);
        }
    }

    // While we have a suggested thread, try to migrate it!
    {
        KThread* suggested = priority_queue.GetSuggestedFront(core_id, priority);
        while (suggested != nullptr) {
            // Check if the suggested thread is the top thread on its core.
            const s32 suggested_core = suggested->GetActiveCore();
            if (KThread* top_on_suggested_core =
                    (suggested_core >= 0) ? priority_queue.GetScheduledFront(suggested_core)
                                          : nullptr;
                top_on_suggested_core != suggested) {
                // If the next thread is a new thread that has been waiting longer than our
                // suggestion, we prefer it to our suggestion.
                if (top_thread != next_thread && next_thread != nullptr &&
                    next_thread->GetLastScheduledTick() < suggested->GetLastScheduledTick()) {
                    suggested = nullptr;
                    break;
                }

                // If we're allowed to do a migration, do one.
                // NOTE: Unlike migrations in UpdateHighestPriorityThread, this moves the suggestion
                // to the front of the queue.
                if (top_on_suggested_core == nullptr ||
                    top_on_suggested_core->GetPriority() >= HighestCoreMigrationAllowedPriority) {
                    suggested->SetActiveCore(core_id);
                    priority_queue.ChangeCore(suggested_core, suggested, true);
                    IncrementScheduledCount(suggested);
                    break;
                }
            }

            // Get the next suggestion.
            suggested = priority_queue.GetSamePriorityNext(core_id, suggested);
        }
    }

    // Now that we might have migrated a thread with the same priority, check if we can do better.
    {
        KThread* best_thread = priority_queue.GetScheduledFront(core_id);
        if (best_thread == GetCurrentThreadPointer(kernel)) {
            best_thread = priority_queue.GetScheduledNext(core_id, best_thread);
        }

        // If the best thread we can choose has a priority the same or worse than ours, try to
        // migrate a higher priority thread.
        if (best_thread != nullptr && best_thread->GetPriority() >= priority) {
            KThread* suggested = priority_queue.GetSuggestedFront(core_id);
            while (suggested != nullptr) {
                // If the suggestion's priority is the same as ours, don't bother.
                if (suggested->GetPriority() >= best_thread->GetPriority()) {
                    break;
                }

                // Check if the suggested thread is the top thread on its core.
                const s32 suggested_core = suggested->GetActiveCore();
                if (KThread* top_on_suggested_core =
                        (suggested_core >= 0) ? priority_queue.GetScheduledFront(suggested_core)
                                              : nullptr;
                    top_on_suggested_core != suggested) {
                    // If we're allowed to do a migration, do one.
                    // NOTE: Unlike migrations in UpdateHighestPriorityThread, this moves the
                    // suggestion to the front of the queue.
                    if (top_on_suggested_core == nullptr ||
                        top_on_suggested_core->GetPriority() >=
                            HighestCoreMigrationAllowedPriority) {
                        suggested->SetActiveCore(core_id);
                        priority_queue.ChangeCore(suggested_core, suggested, true);
                        IncrementScheduledCount(suggested);
                        break;
                    }
                }

                // Get the next suggestion.
                suggested = priority_queue.GetSuggestedNext(core_id, suggested);
            }
        }
    }

    // After a rotation, we need a scheduler update.
    SetSchedulerUpdateNeeded(kernel);
}

void KScheduler::YieldWithoutCoreMigration(KernelCore& kernel) {
    // Validate preconditions.
    ASSERT(CanSchedule(kernel));
    ASSERT(kernel.CurrentProcess() != nullptr);

    // Get the current thread and process.
    KThread& cur_thread = GetCurrentThread(kernel);
    KProcess& cur_process = *kernel.CurrentProcess();

    // If the thread's yield count matches, there's nothing for us to do.
    if (cur_thread.GetYieldScheduleCount() == cur_process.GetScheduledCount()) {
        return;
    }

    // Get a reference to the priority queue.
    auto& priority_queue = GetPriorityQueue(kernel);

    // Perform the yield.
    {
        KScopedSchedulerLock sl{kernel};

        const auto cur_state = cur_thread.GetRawState();
        if (cur_state == ThreadState::Runnable) {
            // Put the current thread at the back of the queue.
            KThread* next_thread = priority_queue.MoveToScheduledBack(std::addressof(cur_thread));
            IncrementScheduledCount(std::addressof(cur_thread));

            // If the next thread is different, we have an update to perform.
            if (next_thread != std::addressof(cur_thread)) {
                SetSchedulerUpdateNeeded(kernel);
            } else {
                // Otherwise, set the thread's yield count so that we won't waste work until the
                // process is scheduled again.
                cur_thread.SetYieldScheduleCount(cur_process.GetScheduledCount());
            }
        }
    }
}

void KScheduler::YieldWithCoreMigration(KernelCore& kernel) {
    // Validate preconditions.
    ASSERT(CanSchedule(kernel));
    ASSERT(kernel.CurrentProcess() != nullptr);

    // Get the current thread and process.
    KThread& cur_thread = GetCurrentThread(kernel);
    KProcess& cur_process = *kernel.CurrentProcess();

    // If the thread's yield count matches, there's nothing for us to do.
    if (cur_thread.GetYieldScheduleCount() == cur_process.GetScheduledCount()) {
        return;
    }

    // Get a reference to the priority queue.
    auto& priority_queue = GetPriorityQueue(kernel);

    // Perform the yield.
    {
        KScopedSchedulerLock sl{kernel};

        const auto cur_state = cur_thread.GetRawState();
        if (cur_state == ThreadState::Runnable) {
            // Get the current active core.
            const s32 core_id = cur_thread.GetActiveCore();

            // Put the current thread at the back of the queue.
            KThread* next_thread = priority_queue.MoveToScheduledBack(std::addressof(cur_thread));
            IncrementScheduledCount(std::addressof(cur_thread));

            // While we have a suggested thread, try to migrate it!
            bool recheck = false;
            KThread* suggested = priority_queue.GetSuggestedFront(core_id);
            while (suggested != nullptr) {
                // Check if the suggested thread is the thread running on its core.
                const s32 suggested_core = suggested->GetActiveCore();

                if (KThread* running_on_suggested_core =
                        (suggested_core >= 0)
                            ? kernel.Scheduler(suggested_core).m_state.highest_priority_thread
                            : nullptr;
                    running_on_suggested_core != suggested) {
                    // If the current thread's priority is higher than our suggestion's we prefer
                    // the next thread to the suggestion. We also prefer the next thread when the
                    // current thread's priority is equal to the suggestions, but the next thread
                    // has been waiting longer.
                    if ((suggested->GetPriority() > cur_thread.GetPriority()) ||
                        (suggested->GetPriority() == cur_thread.GetPriority() &&
                         next_thread != std::addressof(cur_thread) &&
                         next_thread->GetLastScheduledTick() < suggested->GetLastScheduledTick())) {
                        suggested = nullptr;
                        break;
                    }

                    // If we're allowed to do a migration, do one.
                    // NOTE: Unlike migrations in UpdateHighestPriorityThread, this moves the
                    // suggestion to the front of the queue.
                    if (running_on_suggested_core == nullptr ||
                        running_on_suggested_core->GetPriority() >=
                            HighestCoreMigrationAllowedPriority) {
                        suggested->SetActiveCore(core_id);
                        priority_queue.ChangeCore(suggested_core, suggested, true);
                        IncrementScheduledCount(suggested);
                        break;
                    } else {
                        // We couldn't perform a migration, but we should check again on a future
                        // yield.
                        recheck = true;
                    }
                }

                // Get the next suggestion.
                suggested = priority_queue.GetSuggestedNext(core_id, suggested);
            }

            // If we still have a suggestion or the next thread is different, we have an update to
            // perform.
            if (suggested != nullptr || next_thread != std::addressof(cur_thread)) {
                SetSchedulerUpdateNeeded(kernel);
            } else if (!recheck) {
                // Otherwise if we don't need to re-check, set the thread's yield count so that we
                // won't waste work until the process is scheduled again.
                cur_thread.SetYieldScheduleCount(cur_process.GetScheduledCount());
            }
        }
    }
}

void KScheduler::YieldToAnyThread(KernelCore& kernel) {
    // Validate preconditions.
    ASSERT(CanSchedule(kernel));
    ASSERT(kernel.CurrentProcess() != nullptr);

    // Get the current thread and process.
    KThread& cur_thread = GetCurrentThread(kernel);
    KProcess& cur_process = *kernel.CurrentProcess();

    // If the thread's yield count matches, there's nothing for us to do.
    if (cur_thread.GetYieldScheduleCount() == cur_process.GetScheduledCount()) {
        return;
    }

    // Get a reference to the priority queue.
    auto& priority_queue = GetPriorityQueue(kernel);

    // Perform the yield.
    {
        KScopedSchedulerLock sl{kernel};

        const auto cur_state = cur_thread.GetRawState();
        if (cur_state == ThreadState::Runnable) {
            // Get the current active core.
            const s32 core_id = cur_thread.GetActiveCore();

            // Migrate the current thread to core -1.
            cur_thread.SetActiveCore(-1);
            priority_queue.ChangeCore(core_id, std::addressof(cur_thread));
            IncrementScheduledCount(std::addressof(cur_thread));

            // If there's nothing scheduled, we can try to perform a migration.
            if (priority_queue.GetScheduledFront(core_id) == nullptr) {
                // While we have a suggested thread, try to migrate it!
                KThread* suggested = priority_queue.GetSuggestedFront(core_id);
                while (suggested != nullptr) {
                    // Check if the suggested thread is the top thread on its core.
                    const s32 suggested_core = suggested->GetActiveCore();
                    if (KThread* top_on_suggested_core =
                            (suggested_core >= 0) ? priority_queue.GetScheduledFront(suggested_core)
                                                  : nullptr;
                        top_on_suggested_core != suggested) {
                        // If we're allowed to do a migration, do one.
                        if (top_on_suggested_core == nullptr ||
                            top_on_suggested_core->GetPriority() >=
                                HighestCoreMigrationAllowedPriority) {
                            suggested->SetActiveCore(core_id);
                            priority_queue.ChangeCore(suggested_core, suggested);
                            IncrementScheduledCount(suggested);
                        }

                        // Regardless of whether we migrated, we had a candidate, so we're done.
                        break;
                    }

                    // Get the next suggestion.
                    suggested = priority_queue.GetSuggestedNext(core_id, suggested);
                }

                // If the suggestion is different from the current thread, we need to perform an
                // update.
                if (suggested != std::addressof(cur_thread)) {
                    SetSchedulerUpdateNeeded(kernel);
                } else {
                    // Otherwise, set the thread's yield count so that we won't waste work until the
                    // process is scheduled again.
                    cur_thread.SetYieldScheduleCount(cur_process.GetScheduledCount());
                }
            } else {
                // Otherwise, we have an update to perform.
                SetSchedulerUpdateNeeded(kernel);
            }
        }
    }
}

void KScheduler::RescheduleOtherCores(u64 cores_needing_scheduling) {
    if (const u64 core_mask = cores_needing_scheduling & ~(1ULL << m_core_id); core_mask != 0) {
        RescheduleCores(kernel, core_mask);
    }
}

void KScheduler::RescheduleCores(KernelCore& kernel, u64 core_mask) {
    // Send IPI
    for (size_t i = 0; i < Core::Hardware::NUM_CPU_CORES; i++) {
        if (core_mask & (1ULL << i)) {
            kernel.PhysicalCore(i).Interrupt();
        }
    }
}

} // namespace Kernel
