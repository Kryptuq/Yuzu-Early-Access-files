// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// This file references various implementation details from Atmosphere, an open-source firmware for
// the Nintendo Switch. Copyright 2018-2020 Atmosphere-NX.

#include <bit>

#include "common/assert.h"
#include "common/bit_util.h"
#include "common/fiber.h"
#include "common/logging/log.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu_manager.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"
#include "core/hle/kernel/time_manager.h"

namespace Kernel {

static void IncrementScheduledCount(Kernel::KThread* thread) {
    if (auto process = thread->GetOwnerProcess(); process) {
        process->IncrementScheduledCount();
    }
}

void KScheduler::RescheduleCores(KernelCore& kernel, u64 cores_pending_reschedule) {
    auto scheduler = kernel.CurrentScheduler();

    u32 current_core{0xF};
    bool must_context_switch{};
    if (scheduler) {
        current_core = scheduler->core_id;
        // TODO(bunnei): Should be set to true when we deprecate single core
        must_context_switch = !kernel.IsPhantomModeForSingleCore();
    }

    while (cores_pending_reschedule != 0) {
        const auto core = static_cast<u32>(std::countr_zero(cores_pending_reschedule));
        ASSERT(core < Core::Hardware::NUM_CPU_CORES);
        if (!must_context_switch || core != current_core) {
            auto& phys_core = kernel.PhysicalCore(core);
            phys_core.Interrupt();
        } else {
            must_context_switch = true;
        }
        cores_pending_reschedule &= ~(1ULL << core);
    }
    if (must_context_switch) {
        auto core_scheduler = kernel.CurrentScheduler();
        kernel.ExitSVCProfile();
        core_scheduler->RescheduleCurrentCore();
        kernel.EnterSVCProfile();
    }
}

u64 KScheduler::UpdateHighestPriorityThread(KThread* highest_thread) {
    KScopedSpinLock lk{guard};
    if (KThread* prev_highest_thread = state.highest_priority_thread;
        prev_highest_thread != highest_thread) {
        if (prev_highest_thread != nullptr) {
            IncrementScheduledCount(prev_highest_thread);
            prev_highest_thread->SetLastScheduledTick(system.CoreTiming().GetCPUTicks());
        }
        if (state.should_count_idle) {
            if (highest_thread != nullptr) {
                if (KProcess* process = highest_thread->GetOwnerProcess(); process != nullptr) {
                    process->SetRunningThread(core_id, highest_thread, state.idle_count);
                }
            } else {
                state.idle_count++;
            }
        }

        state.highest_priority_thread = highest_thread;
        state.needs_scheduling.store(true);
        return (1ULL << core_id);
    } else {
        return 0;
    }
}

u64 KScheduler::UpdateHighestPriorityThreadsImpl(KernelCore& kernel) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Clear that we need to update.
    ClearSchedulerUpdateNeeded(kernel);

    u64 cores_needing_scheduling = 0, idle_cores = 0;
    KThread* top_threads[Core::Hardware::NUM_CPU_CORES];
    auto& priority_queue = GetPriorityQueue(kernel);

    /// We want to go over all cores, finding the highest priority thread and determining if
    /// scheduling is needed for that core.
    for (size_t core_id = 0; core_id < Core::Hardware::NUM_CPU_CORES; core_id++) {
        KThread* top_thread = priority_queue.GetScheduledFront(static_cast<s32>(core_id));
        if (top_thread != nullptr) {
            // If the thread has no waiters, we need to check if the process has a thread pinned.
            if (top_thread->GetNumKernelWaiters() == 0) {
                if (KProcess* parent = top_thread->GetOwnerProcess(); parent != nullptr) {
                    if (KThread* pinned = parent->GetPinnedThread(static_cast<s32>(core_id));
                        pinned != nullptr && pinned != top_thread) {
                        // We prefer our parent's pinned thread if possible. However, we also don't
                        // want to schedule un-runnable threads.
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
        const auto core_id = static_cast<u32>(std::countr_zero(idle_cores));
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

    return cores_needing_scheduling;
}

void KScheduler::ClearPreviousThread(KernelCore& kernel, KThread* thread) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());
    for (size_t i = 0; i < Core::Hardware::NUM_CPU_CORES; ++i) {
        // Get an atomic reference to the core scheduler's previous thread.
        std::atomic_ref<KThread*> prev_thread(kernel.Scheduler(static_cast<s32>(i)).prev_thread);
        static_assert(std::atomic_ref<KThread*>::is_always_lock_free);

        // Atomically clear the previous thread if it's our target.
        KThread* compare = thread;
        prev_thread.compare_exchange_strong(compare, nullptr);
    }
}

void KScheduler::OnThreadStateChanged(KernelCore& kernel, KThread* thread, ThreadState old_state) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Check if the state has changed, because if it hasn't there's nothing to do.
    const auto cur_state = thread->GetRawState();
    if (cur_state == old_state) {
        return;
    }

    // Update the priority queues.
    if (old_state == ThreadState::Runnable) {
        // If we were previously runnable, then we're not runnable now, and we should remove.
        GetPriorityQueue(kernel).Remove(thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);
    } else if (cur_state == ThreadState::Runnable) {
        // If we're now runnable, then we weren't previously, and we should add.
        GetPriorityQueue(kernel).PushBack(thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);
    }
}

void KScheduler::OnThreadPriorityChanged(KernelCore& kernel, KThread* thread, s32 old_priority) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // If the thread is runnable, we want to change its priority in the queue.
    if (thread->GetRawState() == ThreadState::Runnable) {
        GetPriorityQueue(kernel).ChangePriority(old_priority,
                                                thread == kernel.GetCurrentEmuThread(), thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);
    }
}

void KScheduler::OnThreadAffinityMaskChanged(KernelCore& kernel, KThread* thread,
                                             const KAffinityMask& old_affinity, s32 old_core) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // If the thread is runnable, we want to change its affinity in the queue.
    if (thread->GetRawState() == ThreadState::Runnable) {
        GetPriorityQueue(kernel).ChangeAffinityMask(old_core, old_affinity, thread);
        IncrementScheduledCount(thread);
        SetSchedulerUpdateNeeded(kernel);
    }
}

void KScheduler::RotateScheduledQueue(s32 cpu_core_id, s32 priority) {
    ASSERT(system.GlobalSchedulerContext().IsLocked());

    // Get a reference to the priority queue.
    auto& kernel = system.Kernel();
    auto& priority_queue = GetPriorityQueue(kernel);

    // Rotate the front of the queue to the end.
    KThread* top_thread = priority_queue.GetScheduledFront(cpu_core_id, priority);
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
        KThread* suggested = priority_queue.GetSuggestedFront(cpu_core_id, priority);
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
                    suggested->SetActiveCore(cpu_core_id);
                    priority_queue.ChangeCore(suggested_core, suggested, true);
                    IncrementScheduledCount(suggested);
                    break;
                }
            }

            // Get the next suggestion.
            suggested = priority_queue.GetSamePriorityNext(cpu_core_id, suggested);
        }
    }

    // Now that we might have migrated a thread with the same priority, check if we can do better.

    {
        KThread* best_thread = priority_queue.GetScheduledFront(cpu_core_id);
        if (best_thread == GetCurrentThread()) {
            best_thread = priority_queue.GetScheduledNext(cpu_core_id, best_thread);
        }

        // If the best thread we can choose has a priority the same or worse than ours, try to
        // migrate a higher priority thread.
        if (best_thread != nullptr && best_thread->GetPriority() >= priority) {
            KThread* suggested = priority_queue.GetSuggestedFront(cpu_core_id);
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
                        suggested->SetActiveCore(cpu_core_id);
                        priority_queue.ChangeCore(suggested_core, suggested, true);
                        IncrementScheduledCount(suggested);
                        break;
                    }
                }

                // Get the next suggestion.
                suggested = priority_queue.GetSuggestedNext(cpu_core_id, suggested);
            }
        }
    }

    // After a rotation, we need a scheduler update.
    SetSchedulerUpdateNeeded(kernel);
}

bool KScheduler::CanSchedule(KernelCore& kernel) {
    return kernel.GetCurrentEmuThread()->GetDisableDispatchCount() <= 1;
}

bool KScheduler::IsSchedulerUpdateNeeded(const KernelCore& kernel) {
    return kernel.GlobalSchedulerContext().scheduler_update_needed.load(std::memory_order_acquire);
}

void KScheduler::SetSchedulerUpdateNeeded(KernelCore& kernel) {
    kernel.GlobalSchedulerContext().scheduler_update_needed.store(true, std::memory_order_release);
}

void KScheduler::ClearSchedulerUpdateNeeded(KernelCore& kernel) {
    kernel.GlobalSchedulerContext().scheduler_update_needed.store(false, std::memory_order_release);
}

void KScheduler::DisableScheduling(KernelCore& kernel) {
    // If we are shutting down the kernel, none of this is relevant anymore.
    if (kernel.IsShuttingDown()) {
        return;
    }

    ASSERT(GetCurrentThreadPointer(kernel)->GetDisableDispatchCount() >= 0);
    GetCurrentThreadPointer(kernel)->DisableDispatch();
}

void KScheduler::EnableScheduling(KernelCore& kernel, u64 cores_needing_scheduling) {
    // If we are shutting down the kernel, none of this is relevant anymore.
    if (kernel.IsShuttingDown()) {
        return;
    }

    auto* current_thread = GetCurrentThreadPointer(kernel);

    ASSERT(current_thread->GetDisableDispatchCount() >= 1);

    if (current_thread->GetDisableDispatchCount() > 1) {
        current_thread->EnableDispatch();
    } else {
        RescheduleCores(kernel, cores_needing_scheduling);
    }
}

u64 KScheduler::UpdateHighestPriorityThreads(KernelCore& kernel) {
    if (IsSchedulerUpdateNeeded(kernel)) {
        return UpdateHighestPriorityThreadsImpl(kernel);
    } else {
        return 0;
    }
}

KSchedulerPriorityQueue& KScheduler::GetPriorityQueue(KernelCore& kernel) {
    return kernel.GlobalSchedulerContext().priority_queue;
}

void KScheduler::YieldWithoutCoreMigration(KernelCore& kernel) {
    // Validate preconditions.
    ASSERT(CanSchedule(kernel));
    ASSERT(kernel.CurrentProcess() != nullptr);

    // Get the current thread and process.
    KThread& cur_thread = Kernel::GetCurrentThread(kernel);
    KProcess& cur_process = *kernel.CurrentProcess();

    // If the thread's yield count matches, there's nothing for us to do.
    if (cur_thread.GetYieldScheduleCount() == cur_process.GetScheduledCount()) {
        return;
    }

    // Get a reference to the priority queue.
    auto& priority_queue = GetPriorityQueue(kernel);

    // Perform the yield.
    {
        KScopedSchedulerLock lock(kernel);

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
    KThread& cur_thread = Kernel::GetCurrentThread(kernel);
    KProcess& cur_process = *kernel.CurrentProcess();

    // If the thread's yield count matches, there's nothing for us to do.
    if (cur_thread.GetYieldScheduleCount() == cur_process.GetScheduledCount()) {
        return;
    }

    // Get a reference to the priority queue.
    auto& priority_queue = GetPriorityQueue(kernel);

    // Perform the yield.
    {
        KScopedSchedulerLock lock(kernel);

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
                            ? kernel.Scheduler(suggested_core).state.highest_priority_thread
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
    KThread& cur_thread = Kernel::GetCurrentThread(kernel);
    KProcess& cur_process = *kernel.CurrentProcess();

    // If the thread's yield count matches, there's nothing for us to do.
    if (cur_thread.GetYieldScheduleCount() == cur_process.GetScheduledCount()) {
        return;
    }

    // Get a reference to the priority queue.
    auto& priority_queue = GetPriorityQueue(kernel);

    // Perform the yield.
    {
        KScopedSchedulerLock lock(kernel);

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

KScheduler::KScheduler(Core::System& system_, s32 core_id_) : system{system_}, core_id{core_id_} {
    switch_fiber = std::make_shared<Common::Fiber>(OnSwitch, this);
    state.needs_scheduling.store(true);
    state.interrupt_task_thread_runnable = false;
    state.should_count_idle = false;
    state.idle_count = 0;
    state.idle_thread_stack = nullptr;
    state.highest_priority_thread = nullptr;
}

void KScheduler::Finalize() {
    if (idle_thread) {
        idle_thread->Close();
        idle_thread = nullptr;
    }
}

KScheduler::~KScheduler() {
    ASSERT(!idle_thread);
}

KThread* KScheduler::GetCurrentThread() const {
    if (auto result = current_thread.load(); result) {
        return result;
    }
    return idle_thread;
}

u64 KScheduler::GetLastContextSwitchTicks() const {
    return last_context_switch_time;
}

void KScheduler::RescheduleCurrentCore() {
    ASSERT(GetCurrentThread()->GetDisableDispatchCount() == 1);

    auto& phys_core = system.Kernel().PhysicalCore(core_id);
    if (phys_core.IsInterrupted()) {
        phys_core.ClearInterrupt();
    }

    guard.Lock();
    if (state.needs_scheduling.load()) {
        Schedule();
    } else {
        GetCurrentThread()->EnableDispatch();
        guard.Unlock();
    }
}

void KScheduler::OnThreadStart() {
    SwitchContextStep2();
}

void KScheduler::Unload(KThread* thread) {
    ASSERT(thread);

    LOG_TRACE(Kernel, "core {}, unload thread {}", core_id, thread ? thread->GetName() : "nullptr");

    if (thread->IsCallingSvc()) {
        thread->ClearIsCallingSvc();
    }

    auto& physical_core = system.Kernel().PhysicalCore(core_id);
    if (!physical_core.IsInitialized()) {
        return;
    }

    Core::ARM_Interface& cpu_core = physical_core.ArmInterface();
    cpu_core.SaveContext(thread->GetContext32());
    cpu_core.SaveContext(thread->GetContext64());
    // Save the TPIDR_EL0 system register in case it was modified.
    thread->SetTPIDR_EL0(cpu_core.GetTPIDR_EL0());
    cpu_core.ClearExclusiveState();

    if (!thread->IsTerminationRequested() && thread->GetActiveCore() == core_id) {
        prev_thread = thread;
    } else {
        prev_thread = nullptr;
    }

    thread->context_guard.Unlock();
}

void KScheduler::Reload(KThread* thread) {
    LOG_TRACE(Kernel, "core {}, reload thread {}", core_id, thread ? thread->GetName() : "nullptr");

    if (thread) {
        ASSERT_MSG(thread->GetState() == ThreadState::Runnable, "Thread must be runnable.");

        Core::ARM_Interface& cpu_core = system.ArmInterface(core_id);
        cpu_core.LoadContext(thread->GetContext32());
        cpu_core.LoadContext(thread->GetContext64());
        cpu_core.SetTlsAddress(thread->GetTLSAddress());
        cpu_core.SetTPIDR_EL0(thread->GetTPIDR_EL0());
        cpu_core.ClearExclusiveState();
    }
}

void KScheduler::SwitchContextStep2() {
    // Load context of new thread
    Reload(current_thread.load());

    RescheduleCurrentCore();
}

void KScheduler::ScheduleImpl() {
    KThread* previous_thread = GetCurrentThread();
    KThread* next_thread = state.highest_priority_thread;

    state.needs_scheduling = false;

    // We never want to schedule a null thread, so use the idle thread if we don't have a next.
    if (next_thread == nullptr) {
        next_thread = idle_thread;
    }

    // If we're not actually switching thread, there's nothing to do.
    if (next_thread == current_thread.load()) {
        previous_thread->EnableDispatch();
        guard.Unlock();
        return;
    }

    if (next_thread->GetCurrentCore() != core_id) {
        next_thread->SetCurrentCore(core_id);
    }

    current_thread.store(next_thread);

    KProcess* const previous_process = system.Kernel().CurrentProcess();

    UpdateLastContextSwitchTime(previous_thread, previous_process);

    // Save context for previous thread
    Unload(previous_thread);

    std::shared_ptr<Common::Fiber>* old_context;
    old_context = &previous_thread->GetHostContext();
    guard.Unlock();

    Common::Fiber::YieldTo(*old_context, *switch_fiber);
    /// When a thread wakes up, the scheduler may have changed to other in another core.
    auto& next_scheduler = *system.Kernel().CurrentScheduler();
    next_scheduler.SwitchContextStep2();
}

void KScheduler::OnSwitch(void* this_scheduler) {
    KScheduler* sched = static_cast<KScheduler*>(this_scheduler);
    sched->SwitchToCurrent();
}

void KScheduler::SwitchToCurrent() {
    while (true) {
        {
            KScopedSpinLock lk{guard};
            current_thread.store(state.highest_priority_thread);
            state.needs_scheduling.store(false);
        }
        const auto is_switch_pending = [this] {
            KScopedSpinLock lk{guard};
            return state.needs_scheduling.load();
        };
        do {
            auto next_thread = current_thread.load();
            if (next_thread != nullptr) {
                next_thread->context_guard.Lock();
                if (next_thread->GetRawState() != ThreadState::Runnable) {
                    next_thread->context_guard.Unlock();
                    break;
                }
                if (next_thread->GetActiveCore() != core_id) {
                    next_thread->context_guard.Unlock();
                    break;
                }
            }
            auto thread = next_thread ? next_thread : idle_thread;
            Common::Fiber::YieldTo(switch_fiber, *thread->GetHostContext());
        } while (!is_switch_pending());
    }
}

void KScheduler::UpdateLastContextSwitchTime(KThread* thread, KProcess* process) {
    const u64 prev_switch_ticks = last_context_switch_time;
    const u64 most_recent_switch_ticks = system.CoreTiming().GetCPUTicks();
    const u64 update_ticks = most_recent_switch_ticks - prev_switch_ticks;

    if (thread != nullptr) {
        thread->AddCpuTime(core_id, update_ticks);
    }

    if (process != nullptr) {
        process->UpdateCPUTimeTicks(update_ticks);
    }

    last_context_switch_time = most_recent_switch_ticks;
}

void KScheduler::Initialize() {
    idle_thread = KThread::Create(system.Kernel());
    ASSERT(KThread::InitializeIdleThread(system, idle_thread, core_id).IsSuccess());
    idle_thread->SetName(fmt::format("IdleThread:{}", core_id));
}

KScopedSchedulerLock::KScopedSchedulerLock(KernelCore& kernel)
    : KScopedLock(kernel.GlobalSchedulerContext().SchedulerLock()) {}

KScopedSchedulerLock::~KScopedSchedulerLock() = default;

} // namespace Kernel
