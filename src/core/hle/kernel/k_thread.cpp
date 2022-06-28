// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <optional>
#include <vector>

#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/fiber.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/cpu_manager.h"
#include "core/hardware_properties.h"
#include "core/hle/kernel/k_condition_variable.h"
#include "core/hle/kernel/k_handle_table.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_scheduler_lock_and_sleep.h"
#include "core/hle/kernel/k_system_control.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_thread_queue.h"
#include "core/hle/kernel/k_worker_task_manager.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"
#include "core/hle/result.h"
#include "core/memory.h"

#ifdef ARCHITECTURE_x86_64
#include "core/arm/dynarmic/arm_dynarmic_32.h"
#endif

namespace {
static void ResetThreadContext32(Core::ARM_Interface::ThreadContext32& context, u32 stack_top,
                                 u32 entry_point, u32 arg) {
    context = {};
    context.cpu_registers[0] = arg;
    context.cpu_registers[15] = entry_point;
    context.cpu_registers[13] = stack_top;
}

static void ResetThreadContext64(Core::ARM_Interface::ThreadContext64& context, VAddr stack_top,
                                 VAddr entry_point, u64 arg) {
    context = {};
    context.cpu_registers[0] = arg;
    context.cpu_registers[18] = Kernel::KSystemControl::GenerateRandomU64() | 1;
    context.pc = entry_point;
    context.sp = stack_top;
    // TODO(merry): Perform a hardware test to determine the below value.
    context.fpcr = 0;
}
} // namespace

namespace Kernel {

namespace {

struct ThreadLocalRegion {
    static constexpr std::size_t MessageBufferSize = 0x100;
    std::array<u32, MessageBufferSize / sizeof(u32)> message_buffer;
    std::atomic_uint16_t disable_count;
    std::atomic_uint16_t interrupt_flag;
};

class ThreadQueueImplForKThreadSleep final : public KThreadQueueWithoutEndWait {
public:
    explicit ThreadQueueImplForKThreadSleep(KernelCore& kernel_)
        : KThreadQueueWithoutEndWait(kernel_) {}
};

class ThreadQueueImplForKThreadSetProperty final : public KThreadQueue {
public:
    explicit ThreadQueueImplForKThreadSetProperty(KernelCore& kernel_, KThread::WaiterList* wl)
        : KThreadQueue(kernel_), m_wait_list(wl) {}

    void CancelWait(KThread* waiting_thread, ResultCode wait_result,
                    bool cancel_timer_task) override {
        // Remove the thread from the wait list.
        m_wait_list->erase(m_wait_list->iterator_to(*waiting_thread));

        // Invoke the base cancel wait handler.
        KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
    }

private:
    KThread::WaiterList* m_wait_list;
};

} // namespace

KThread::KThread(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_}, activity_pause_lock{kernel_} {}
KThread::~KThread() = default;

ResultCode KThread::Initialize(KThreadFunction func, uintptr_t arg, VAddr user_stack_top, s32 prio,
                               s32 virt_core, KProcess* owner, ThreadType type) {
    // Assert parameters are valid.
    ASSERT((type == ThreadType::Main) || (type == ThreadType::Dummy) ||
           (Svc::HighestThreadPriority <= prio && prio <= Svc::LowestThreadPriority));
    ASSERT((owner != nullptr) || (type != ThreadType::User));
    ASSERT(0 <= virt_core && virt_core < static_cast<s32>(Common::BitSize<u64>()));

    // Convert the virtual core to a physical core.
    const s32 phys_core = Core::Hardware::VirtualToPhysicalCoreMap[virt_core];
    ASSERT(0 <= phys_core && phys_core < static_cast<s32>(Core::Hardware::NUM_CPU_CORES));

    // First, clear the TLS address.
    tls_address = {};

    // Next, assert things based on the type.
    switch (type) {
    case ThreadType::Main:
        ASSERT(arg == 0);
        [[fallthrough]];
    case ThreadType::HighPriority:
        [[fallthrough]];
    case ThreadType::Dummy:
        [[fallthrough]];
    case ThreadType::User:
        ASSERT(((owner == nullptr) ||
                (owner->GetCoreMask() | (1ULL << virt_core)) == owner->GetCoreMask()));
        ASSERT(((owner == nullptr) ||
                (owner->GetPriorityMask() | (1ULL << prio)) == owner->GetPriorityMask()));
        break;
    case ThreadType::Kernel:
        UNIMPLEMENTED();
        break;
    default:
        ASSERT_MSG(false, "KThread::Initialize: Unknown ThreadType {}", static_cast<u32>(type));
        break;
    }
    thread_type = type;

    // Set the ideal core ID and affinity mask.
    virtual_ideal_core_id = virt_core;
    physical_ideal_core_id = phys_core;
    virtual_affinity_mask = 1ULL << virt_core;
    physical_affinity_mask.SetAffinity(phys_core, true);

    // Set the thread state.
    thread_state = (type == ThreadType::Main) ? ThreadState::Runnable : ThreadState::Initialized;

    // Set TLS address.
    tls_address = 0;

    // Set parent and condvar tree.
    parent = nullptr;
    condvar_tree = nullptr;

    // Set sync booleans.
    signaled = false;
    termination_requested = false;
    wait_cancelled = false;
    cancellable = false;

    // Set core ID and wait result.
    core_id = phys_core;
    wait_result = ResultNoSynchronizationObject;

    // Set priorities.
    priority = prio;
    base_priority = prio;

    // Initialize sleeping queue.
    wait_queue = nullptr;

    // Set suspend flags.
    suspend_request_flags = 0;
    suspend_allowed_flags = static_cast<u32>(ThreadState::SuspendFlagMask);

    // We're neither debug attached, nor are we nesting our priority inheritance.
    debug_attached = false;
    priority_inheritance_count = 0;

    // We haven't been scheduled, and we have done no light IPC.
    schedule_count = -1;
    last_scheduled_tick = 0;
    light_ipc_data = nullptr;

    // We're not waiting for a lock, and we haven't disabled migration.
    lock_owner = nullptr;
    num_core_migration_disables = 0;

    // We have no waiters, but we do have an entrypoint.
    num_kernel_waiters = 0;

    // Set our current core id.
    current_core_id = phys_core;

    // We haven't released our resource limit hint, and we've spent no time on the cpu.
    resource_limit_release_hint = false;
    cpu_time = 0;

    // Set debug context.
    stack_top = user_stack_top;
    argument = arg;

    // Clear our stack parameters.
    std::memset(static_cast<void*>(std::addressof(GetStackParameters())), 0,
                sizeof(StackParameters));

    // Set parent, if relevant.
    if (owner != nullptr) {
        // Setup the TLS, if needed.
        if (type == ThreadType::User) {
            R_TRY(owner->CreateThreadLocalRegion(std::addressof(tls_address)));
        }

        parent = owner;
        parent->Open();
    }

    // Initialize thread context.
    ResetThreadContext64(thread_context_64, user_stack_top, func, arg);
    ResetThreadContext32(thread_context_32, static_cast<u32>(user_stack_top),
                         static_cast<u32>(func), static_cast<u32>(arg));

    // Setup the stack parameters.
    StackParameters& sp = GetStackParameters();
    sp.cur_thread = this;
    sp.disable_count = 1;
    SetInExceptionHandler();

    // Set thread ID.
    thread_id = kernel.CreateNewThreadID();

    // We initialized!
    initialized = true;

    // Register ourselves with our parent process.
    if (parent != nullptr) {
        parent->RegisterThread(this);
        if (parent->IsSuspended()) {
            RequestSuspend(SuspendType::Process);
        }
    }

    return ResultSuccess;
}

ResultCode KThread::InitializeThread(KThread* thread, KThreadFunction func, uintptr_t arg,
                                     VAddr user_stack_top, s32 prio, s32 core, KProcess* owner,
                                     ThreadType type, std::function<void(void*)>&& init_func,
                                     void* init_func_parameter) {
    // Initialize the thread.
    R_TRY(thread->Initialize(func, arg, user_stack_top, prio, core, owner, type));

    // Initialize emulation parameters.
    thread->host_context =
        std::make_shared<Common::Fiber>(std::move(init_func), init_func_parameter);
    thread->is_single_core = !Settings::values.use_multi_core.GetValue();

    return ResultSuccess;
}

ResultCode KThread::InitializeDummyThread(KThread* thread) {
    return thread->Initialize({}, {}, {}, DummyThreadPriority, 3, {}, ThreadType::Dummy);
}

ResultCode KThread::InitializeIdleThread(Core::System& system, KThread* thread, s32 virt_core) {
    return InitializeThread(thread, {}, {}, {}, IdleThreadPriority, virt_core, {}, ThreadType::Main,
                            Core::CpuManager::GetIdleThreadStartFunc(),
                            system.GetCpuManager().GetStartFuncParameter());
}

ResultCode KThread::InitializeHighPriorityThread(Core::System& system, KThread* thread,
                                                 KThreadFunction func, uintptr_t arg,
                                                 s32 virt_core) {
    return InitializeThread(thread, func, arg, {}, {}, virt_core, nullptr, ThreadType::HighPriority,
                            Core::CpuManager::GetShutdownThreadStartFunc(),
                            system.GetCpuManager().GetStartFuncParameter());
}

ResultCode KThread::InitializeUserThread(Core::System& system, KThread* thread,
                                         KThreadFunction func, uintptr_t arg, VAddr user_stack_top,
                                         s32 prio, s32 virt_core, KProcess* owner) {
    system.Kernel().GlobalSchedulerContext().AddThread(thread);
    return InitializeThread(thread, func, arg, user_stack_top, prio, virt_core, owner,
                            ThreadType::User, Core::CpuManager::GetGuestThreadStartFunc(),
                            system.GetCpuManager().GetStartFuncParameter());
}

void KThread::PostDestroy(uintptr_t arg) {
    KProcess* owner = reinterpret_cast<KProcess*>(arg & ~1ULL);
    const bool resource_limit_release_hint = (arg & 1);
    const s64 hint_value = (resource_limit_release_hint ? 0 : 1);
    if (owner != nullptr) {
        owner->GetResourceLimit()->Release(LimitableResource::Threads, 1, hint_value);
        owner->Close();
    }
}

void KThread::Finalize() {
    // If the thread has an owner process, unregister it.
    if (parent != nullptr) {
        parent->UnregisterThread(this);
    }

    // If the thread has a local region, delete it.
    if (tls_address != 0) {
        ASSERT(parent->DeleteThreadLocalRegion(tls_address).IsSuccess());
    }

    // Release any waiters.
    {
        ASSERT(lock_owner == nullptr);
        KScopedSchedulerLock sl{kernel};

        auto it = waiter_list.begin();
        while (it != waiter_list.end()) {
            // Clear the lock owner
            it->SetLockOwner(nullptr);

            // Erase the waiter from our list.
            it = waiter_list.erase(it);

            // Cancel the thread's wait.
            it->CancelWait(ResultInvalidState, true);
        }
    }

    // Release host emulation members.
    host_context.reset();

    // Perform inherited finalization.
    KSynchronizationObject::Finalize();
}

bool KThread::IsSignaled() const {
    return signaled;
}

void KThread::OnTimer() {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // If we're waiting, cancel the wait.
    if (GetState() == ThreadState::Waiting) {
        wait_queue->CancelWait(this, ResultTimedOut, false);
    }
}

void KThread::StartTermination() {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Release user exception and unpin, if relevant.
    if (parent != nullptr) {
        parent->ReleaseUserException(this);
        if (parent->GetPinnedThread(GetCurrentCoreId(kernel)) == this) {
            parent->UnpinCurrentThread(core_id);
        }
    }

    // Set state to terminated.
    SetState(ThreadState::Terminated);

    // Clear the thread's status as running in parent.
    if (parent != nullptr) {
        parent->ClearRunningThread(this);
    }

    // Signal.
    signaled = true;
    KSynchronizationObject::NotifyAvailable();

    // Clear previous thread in KScheduler.
    KScheduler::ClearPreviousThread(kernel, this);

    // Register terminated dpc flag.
    RegisterDpc(DpcFlag::Terminated);
}

void KThread::FinishTermination() {
    // Ensure that the thread is not executing on any core.
    if (parent != nullptr) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(Core::Hardware::NUM_CPU_CORES); ++i) {
            KThread* core_thread{};
            do {
                core_thread = kernel.Scheduler(i).GetSchedulerCurrentThread();
            } while (core_thread == this);
        }
    }

    // Close the thread.
    this->Close();
}

void KThread::DoWorkerTaskImpl() {
    // Finish the termination that was begun by Exit().
    this->FinishTermination();
}

void KThread::Pin(s32 current_core) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Set ourselves as pinned.
    GetStackParameters().is_pinned = true;

    // Disable core migration.
    ASSERT(num_core_migration_disables == 0);
    {
        ++num_core_migration_disables;

        // Save our ideal state to restore when we're unpinned.
        original_physical_ideal_core_id = physical_ideal_core_id;
        original_physical_affinity_mask = physical_affinity_mask;

        // Bind ourselves to this core.
        const s32 active_core = GetActiveCore();

        SetActiveCore(current_core);
        physical_ideal_core_id = current_core;
        physical_affinity_mask.SetAffinityMask(1ULL << current_core);

        if (active_core != current_core || physical_affinity_mask.GetAffinityMask() !=
                                               original_physical_affinity_mask.GetAffinityMask()) {
            KScheduler::OnThreadAffinityMaskChanged(kernel, this, original_physical_affinity_mask,
                                                    active_core);
        }
    }

    // Disallow performing thread suspension.
    {
        // Update our allow flags.
        suspend_allowed_flags &= ~(1 << (static_cast<u32>(SuspendType::Thread) +
                                         static_cast<u32>(ThreadState::SuspendShift)));

        // Update our state.
        UpdateState();
    }

    // TODO(bunnei): Update our SVC access permissions.
    ASSERT(parent != nullptr);
}

void KThread::Unpin() {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Set ourselves as unpinned.
    GetStackParameters().is_pinned = false;

    // Enable core migration.
    ASSERT(num_core_migration_disables == 1);
    {
        num_core_migration_disables--;

        // Restore our original state.
        const KAffinityMask old_mask = physical_affinity_mask;

        physical_ideal_core_id = original_physical_ideal_core_id;
        physical_affinity_mask = original_physical_affinity_mask;

        if (physical_affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
            const s32 active_core = GetActiveCore();

            if (!physical_affinity_mask.GetAffinity(active_core)) {
                if (physical_ideal_core_id >= 0) {
                    SetActiveCore(physical_ideal_core_id);
                } else {
                    SetActiveCore(static_cast<s32>(
                        Common::BitSize<u64>() - 1 -
                        std::countl_zero(physical_affinity_mask.GetAffinityMask())));
                }
            }
            KScheduler::OnThreadAffinityMaskChanged(kernel, this, old_mask, active_core);
        }
    }

    // Allow performing thread suspension (if termination hasn't been requested).
    if (!IsTerminationRequested()) {
        // Update our allow flags.
        suspend_allowed_flags |= (1 << (static_cast<u32>(SuspendType::Thread) +
                                        static_cast<u32>(ThreadState::SuspendShift)));

        // Update our state.
        UpdateState();
    }

    // TODO(bunnei): Update our SVC access permissions.
    ASSERT(parent != nullptr);

    // Resume any threads that began waiting on us while we were pinned.
    for (auto it = pinned_waiter_list.begin(); it != pinned_waiter_list.end(); ++it) {
        it->EndWait(ResultSuccess);
    }
}

u16 KThread::GetUserDisableCount() const {
    if (!IsUserThread()) {
        // We only emulate TLS for user threads
        return {};
    }

    auto& memory = kernel.System().Memory();
    return memory.Read16(tls_address + offsetof(ThreadLocalRegion, disable_count));
}

void KThread::SetInterruptFlag() {
    if (!IsUserThread()) {
        // We only emulate TLS for user threads
        return;
    }

    auto& memory = kernel.System().Memory();
    memory.Write16(tls_address + offsetof(ThreadLocalRegion, interrupt_flag), 1);
}

void KThread::ClearInterruptFlag() {
    if (!IsUserThread()) {
        // We only emulate TLS for user threads
        return;
    }

    auto& memory = kernel.System().Memory();
    memory.Write16(tls_address + offsetof(ThreadLocalRegion, interrupt_flag), 0);
}

ResultCode KThread::GetCoreMask(s32* out_ideal_core, u64* out_affinity_mask) {
    KScopedSchedulerLock sl{kernel};

    // Get the virtual mask.
    *out_ideal_core = virtual_ideal_core_id;
    *out_affinity_mask = virtual_affinity_mask;

    return ResultSuccess;
}

ResultCode KThread::GetPhysicalCoreMask(s32* out_ideal_core, u64* out_affinity_mask) {
    KScopedSchedulerLock sl{kernel};
    ASSERT(num_core_migration_disables >= 0);

    // Select between core mask and original core mask.
    if (num_core_migration_disables == 0) {
        *out_ideal_core = physical_ideal_core_id;
        *out_affinity_mask = physical_affinity_mask.GetAffinityMask();
    } else {
        *out_ideal_core = original_physical_ideal_core_id;
        *out_affinity_mask = original_physical_affinity_mask.GetAffinityMask();
    }

    return ResultSuccess;
}

ResultCode KThread::SetCoreMask(s32 core_id_, u64 v_affinity_mask) {
    ASSERT(parent != nullptr);
    ASSERT(v_affinity_mask != 0);
    KScopedLightLock lk(activity_pause_lock);

    // Set the core mask.
    u64 p_affinity_mask = 0;
    {
        KScopedSchedulerLock sl(kernel);
        ASSERT(num_core_migration_disables >= 0);

        // If we're updating, set our ideal virtual core.
        if (core_id_ != Svc::IdealCoreNoUpdate) {
            virtual_ideal_core_id = core_id_;
        } else {
            // Preserve our ideal core id.
            core_id_ = virtual_ideal_core_id;
            R_UNLESS(((1ULL << core_id_) & v_affinity_mask) != 0, ResultInvalidCombination);
        }

        // Set our affinity mask.
        virtual_affinity_mask = v_affinity_mask;

        // Translate the virtual core to a physical core.
        if (core_id_ >= 0) {
            core_id_ = Core::Hardware::VirtualToPhysicalCoreMap[core_id_];
        }

        // Translate the virtual affinity mask to a physical one.
        while (v_affinity_mask != 0) {
            const u64 next = std::countr_zero(v_affinity_mask);
            v_affinity_mask &= ~(1ULL << next);
            p_affinity_mask |= (1ULL << Core::Hardware::VirtualToPhysicalCoreMap[next]);
        }

        // If we haven't disabled migration, perform an affinity change.
        if (num_core_migration_disables == 0) {
            const KAffinityMask old_mask = physical_affinity_mask;

            // Set our new ideals.
            physical_ideal_core_id = core_id_;
            physical_affinity_mask.SetAffinityMask(p_affinity_mask);

            if (physical_affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
                const s32 active_core = GetActiveCore();

                if (active_core >= 0 && !physical_affinity_mask.GetAffinity(active_core)) {
                    const s32 new_core = static_cast<s32>(
                        physical_ideal_core_id >= 0
                            ? physical_ideal_core_id
                            : Common::BitSize<u64>() - 1 -
                                  std::countl_zero(physical_affinity_mask.GetAffinityMask()));
                    SetActiveCore(new_core);
                }
                KScheduler::OnThreadAffinityMaskChanged(kernel, this, old_mask, active_core);
            }
        } else {
            // Otherwise, we edit the original affinity for restoration later.
            original_physical_ideal_core_id = core_id_;
            original_physical_affinity_mask.SetAffinityMask(p_affinity_mask);
        }
    }

    // Update the pinned waiter list.
    ThreadQueueImplForKThreadSetProperty wait_queue_(kernel, std::addressof(pinned_waiter_list));
    {
        bool retry_update{};
        do {
            // Lock the scheduler.
            KScopedSchedulerLock sl(kernel);

            // Don't do any further management if our termination has been requested.
            R_SUCCEED_IF(IsTerminationRequested());

            // By default, we won't need to retry.
            retry_update = false;

            // Check if the thread is currently running.
            bool thread_is_current{};
            s32 thread_core;
            for (thread_core = 0; thread_core < static_cast<s32>(Core::Hardware::NUM_CPU_CORES);
                 ++thread_core) {
                if (kernel.Scheduler(thread_core).GetSchedulerCurrentThread() == this) {
                    thread_is_current = true;
                    break;
                }
            }

            // If the thread is currently running, check whether it's no longer allowed under the
            // new mask.
            if (thread_is_current && ((1ULL << thread_core) & p_affinity_mask) == 0) {
                // If the thread is pinned, we want to wait until it's not pinned.
                if (GetStackParameters().is_pinned) {
                    // Verify that the current thread isn't terminating.
                    R_UNLESS(!GetCurrentThread(kernel).IsTerminationRequested(),
                             ResultTerminationRequested);

                    // Wait until the thread isn't pinned any more.
                    pinned_waiter_list.push_back(GetCurrentThread(kernel));
                    GetCurrentThread(kernel).BeginWait(std::addressof(wait_queue_));
                } else {
                    // If the thread isn't pinned, release the scheduler lock and retry until it's
                    // not current.
                    retry_update = true;
                }
            }
        } while (retry_update);
    }

    return ResultSuccess;
}

void KThread::SetBasePriority(s32 value) {
    ASSERT(Svc::HighestThreadPriority <= value && value <= Svc::LowestThreadPriority);

    KScopedSchedulerLock sl{kernel};

    // Change our base priority.
    base_priority = value;

    // Perform a priority restoration.
    RestorePriority(kernel, this);
}

void KThread::RequestSuspend(SuspendType type) {
    KScopedSchedulerLock sl{kernel};

    // Note the request in our flags.
    suspend_request_flags |=
        (1u << (static_cast<u32>(ThreadState::SuspendShift) + static_cast<u32>(type)));

    // Try to perform the suspend.
    TrySuspend();
}

void KThread::Resume(SuspendType type) {
    KScopedSchedulerLock sl{kernel};

    // Clear the request in our flags.
    suspend_request_flags &=
        ~(1u << (static_cast<u32>(ThreadState::SuspendShift) + static_cast<u32>(type)));

    // Update our state.
    this->UpdateState();
}

void KThread::WaitCancel() {
    KScopedSchedulerLock sl{kernel};

    // Check if we're waiting and cancellable.
    if (this->GetState() == ThreadState::Waiting && cancellable) {
        wait_cancelled = false;
        wait_queue->CancelWait(this, ResultCancelled, true);
    } else {
        // Otherwise, note that we cancelled a wait.
        wait_cancelled = true;
    }
}

void KThread::TrySuspend() {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());
    ASSERT(IsSuspendRequested());

    // Ensure that we have no waiters.
    if (GetNumKernelWaiters() > 0) {
        return;
    }
    ASSERT(GetNumKernelWaiters() == 0);

    // Perform the suspend.
    this->UpdateState();
}

void KThread::UpdateState() {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Set our suspend flags in state.
    const ThreadState old_state = thread_state.load(std::memory_order_relaxed);
    const auto new_state =
        static_cast<ThreadState>(this->GetSuspendFlags()) | (old_state & ThreadState::Mask);
    thread_state.store(new_state, std::memory_order_relaxed);

    // Note the state change in scheduler.
    if (new_state != old_state) {
        KScheduler::OnThreadStateChanged(kernel, this, old_state);
    }
}

void KThread::Continue() {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Clear our suspend flags in state.
    const ThreadState old_state = thread_state.load(std::memory_order_relaxed);
    thread_state.store(old_state & ThreadState::Mask, std::memory_order_relaxed);

    // Note the state change in scheduler.
    KScheduler::OnThreadStateChanged(kernel, this, old_state);
}

void KThread::WaitUntilSuspended() {
    // Make sure we have a suspend requested.
    ASSERT(IsSuspendRequested());

    // Loop until the thread is not executing on any core.
    for (std::size_t i = 0; i < static_cast<std::size_t>(Core::Hardware::NUM_CPU_CORES); ++i) {
        KThread* core_thread{};
        do {
            core_thread = kernel.Scheduler(i).GetSchedulerCurrentThread();
        } while (core_thread == this);
    }
}

ResultCode KThread::SetActivity(Svc::ThreadActivity activity) {
    // Lock ourselves.
    KScopedLightLock lk(activity_pause_lock);

    // Set the activity.
    {
        // Lock the scheduler.
        KScopedSchedulerLock sl(kernel);

        // Verify our state.
        const auto cur_state = this->GetState();
        R_UNLESS((cur_state == ThreadState::Waiting || cur_state == ThreadState::Runnable),
                 ResultInvalidState);

        // Either pause or resume.
        if (activity == Svc::ThreadActivity::Paused) {
            // Verify that we're not suspended.
            R_UNLESS(!this->IsSuspendRequested(SuspendType::Thread), ResultInvalidState);

            // Suspend.
            this->RequestSuspend(SuspendType::Thread);
        } else {
            ASSERT(activity == Svc::ThreadActivity::Runnable);

            // Verify that we're suspended.
            R_UNLESS(this->IsSuspendRequested(SuspendType::Thread), ResultInvalidState);

            // Resume.
            this->Resume(SuspendType::Thread);
        }
    }

    // If the thread is now paused, update the pinned waiter list.
    if (activity == Svc::ThreadActivity::Paused) {
        ThreadQueueImplForKThreadSetProperty wait_queue_(kernel,
                                                         std::addressof(pinned_waiter_list));

        bool thread_is_current;
        do {
            // Lock the scheduler.
            KScopedSchedulerLock sl(kernel);

            // Don't do any further management if our termination has been requested.
            R_SUCCEED_IF(this->IsTerminationRequested());

            // By default, treat the thread as not current.
            thread_is_current = false;

            // Check whether the thread is pinned.
            if (this->GetStackParameters().is_pinned) {
                // Verify that the current thread isn't terminating.
                R_UNLESS(!GetCurrentThread(kernel).IsTerminationRequested(),
                         ResultTerminationRequested);

                // Wait until the thread isn't pinned any more.
                pinned_waiter_list.push_back(GetCurrentThread(kernel));
                GetCurrentThread(kernel).BeginWait(std::addressof(wait_queue_));
            } else {
                // Check if the thread is currently running.
                // If it is, we'll need to retry.
                for (auto i = 0; i < static_cast<s32>(Core::Hardware::NUM_CPU_CORES); ++i) {
                    if (kernel.Scheduler(i).GetSchedulerCurrentThread() == this) {
                        thread_is_current = true;
                        break;
                    }
                }
            }
        } while (thread_is_current);
    }

    return ResultSuccess;
}

ResultCode KThread::GetThreadContext3(std::vector<u8>& out) {
    // Lock ourselves.
    KScopedLightLock lk{activity_pause_lock};

    // Get the context.
    {
        // Lock the scheduler.
        KScopedSchedulerLock sl{kernel};

        // Verify that we're suspended.
        R_UNLESS(IsSuspendRequested(SuspendType::Thread), ResultInvalidState);

        // If we're not terminating, get the thread's user context.
        if (!IsTerminationRequested()) {
            if (parent->Is64BitProcess()) {
                // Mask away mode bits, interrupt bits, IL bit, and other reserved bits.
                auto context = GetContext64();
                context.pstate &= 0xFF0FFE20;

                out.resize(sizeof(context));
                std::memcpy(out.data(), &context, sizeof(context));
            } else {
                // Mask away mode bits, interrupt bits, IL bit, and other reserved bits.
                auto context = GetContext32();
                context.cpsr &= 0xFF0FFE20;

                out.resize(sizeof(context));
                std::memcpy(out.data(), &context, sizeof(context));
            }
        }
    }

    return ResultSuccess;
}

void KThread::AddWaiterImpl(KThread* thread) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Find the right spot to insert the waiter.
    auto it = waiter_list.begin();
    while (it != waiter_list.end()) {
        if (it->GetPriority() > thread->GetPriority()) {
            break;
        }
        it++;
    }

    // Keep track of how many kernel waiters we have.
    if (IsKernelAddressKey(thread->GetAddressKey())) {
        ASSERT((num_kernel_waiters++) >= 0);
        KScheduler::SetSchedulerUpdateNeeded(kernel);
    }

    // Insert the waiter.
    waiter_list.insert(it, *thread);
    thread->SetLockOwner(this);
}

void KThread::RemoveWaiterImpl(KThread* thread) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Keep track of how many kernel waiters we have.
    if (IsKernelAddressKey(thread->GetAddressKey())) {
        ASSERT((num_kernel_waiters--) > 0);
        KScheduler::SetSchedulerUpdateNeeded(kernel);
    }

    // Remove the waiter.
    waiter_list.erase(waiter_list.iterator_to(*thread));
    thread->SetLockOwner(nullptr);
}

void KThread::RestorePriority(KernelCore& kernel_ctx, KThread* thread) {
    ASSERT(kernel_ctx.GlobalSchedulerContext().IsLocked());

    while (true) {
        // We want to inherit priority where possible.
        s32 new_priority = thread->GetBasePriority();
        if (thread->HasWaiters()) {
            new_priority = std::min(new_priority, thread->waiter_list.front().GetPriority());
        }

        // If the priority we would inherit is not different from ours, don't do anything.
        if (new_priority == thread->GetPriority()) {
            return;
        }

        // Ensure we don't violate condition variable red black tree invariants.
        if (auto* cv_tree = thread->GetConditionVariableTree(); cv_tree != nullptr) {
            BeforeUpdatePriority(kernel_ctx, cv_tree, thread);
        }

        // Change the priority.
        const s32 old_priority = thread->GetPriority();
        thread->SetPriority(new_priority);

        // Restore the condition variable, if relevant.
        if (auto* cv_tree = thread->GetConditionVariableTree(); cv_tree != nullptr) {
            AfterUpdatePriority(kernel_ctx, cv_tree, thread);
        }

        // Update the scheduler.
        KScheduler::OnThreadPriorityChanged(kernel_ctx, thread, old_priority);

        // Keep the lock owner up to date.
        KThread* lock_owner = thread->GetLockOwner();
        if (lock_owner == nullptr) {
            return;
        }

        // Update the thread in the lock owner's sorted list, and continue inheriting.
        lock_owner->RemoveWaiterImpl(thread);
        lock_owner->AddWaiterImpl(thread);
        thread = lock_owner;
    }
}

void KThread::AddWaiter(KThread* thread) {
    AddWaiterImpl(thread);
    RestorePriority(kernel, this);
}

void KThread::RemoveWaiter(KThread* thread) {
    RemoveWaiterImpl(thread);
    RestorePriority(kernel, this);
}

KThread* KThread::RemoveWaiterByKey(s32* out_num_waiters, VAddr key) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    s32 num_waiters{};
    KThread* next_lock_owner{};
    auto it = waiter_list.begin();
    while (it != waiter_list.end()) {
        if (it->GetAddressKey() == key) {
            KThread* thread = std::addressof(*it);

            // Keep track of how many kernel waiters we have.
            if (IsKernelAddressKey(thread->GetAddressKey())) {
                ASSERT((num_kernel_waiters--) > 0);
                KScheduler::SetSchedulerUpdateNeeded(kernel);
            }
            it = waiter_list.erase(it);

            // Update the next lock owner.
            if (next_lock_owner == nullptr) {
                next_lock_owner = thread;
                next_lock_owner->SetLockOwner(nullptr);
            } else {
                next_lock_owner->AddWaiterImpl(thread);
            }
            num_waiters++;
        } else {
            it++;
        }
    }

    // Do priority updates, if we have a next owner.
    if (next_lock_owner) {
        RestorePriority(kernel, this);
        RestorePriority(kernel, next_lock_owner);
    }

    // Return output.
    *out_num_waiters = num_waiters;
    return next_lock_owner;
}

ResultCode KThread::Run() {
    while (true) {
        KScopedSchedulerLock lk{kernel};

        // If either this thread or the current thread are requesting termination, note it.
        R_UNLESS(!IsTerminationRequested(), ResultTerminationRequested);
        R_UNLESS(!GetCurrentThread(kernel).IsTerminationRequested(), ResultTerminationRequested);

        // Ensure our thread state is correct.
        R_UNLESS(GetState() == ThreadState::Initialized, ResultInvalidState);

        // If the current thread has been asked to suspend, suspend it and retry.
        if (GetCurrentThread(kernel).IsSuspended()) {
            GetCurrentThread(kernel).UpdateState();
            continue;
        }

        // If we're not a kernel thread and we've been asked to suspend, suspend ourselves.
        if (KProcess* owner = this->GetOwnerProcess(); owner != nullptr) {
            if (IsUserThread() && IsSuspended()) {
                this->UpdateState();
            }
            owner->IncrementRunningThreadCount();
        }

        // Set our state and finish.
        SetState(ThreadState::Runnable);

        return ResultSuccess;
    }
}

void KThread::Exit() {
    ASSERT(this == GetCurrentThreadPointer(kernel));

    // Release the thread resource hint, running thread count from parent.
    if (parent != nullptr) {
        parent->GetResourceLimit()->Release(Kernel::LimitableResource::Threads, 0, 1);
        resource_limit_release_hint = true;
        parent->DecrementRunningThreadCount();
    }

    // Perform termination.
    {
        KScopedSchedulerLock sl{kernel};

        // Disallow all suspension.
        suspend_allowed_flags = 0;
        this->UpdateState();

        // Disallow all suspension.
        suspend_allowed_flags = 0;

        // Start termination.
        StartTermination();

        // Register the thread as a work task.
        KWorkerTaskManager::AddTask(kernel, KWorkerTaskManager::WorkerType::Exit, this);
    }
}

ResultCode KThread::Sleep(s64 timeout) {
    ASSERT(!kernel.GlobalSchedulerContext().IsLocked());
    ASSERT(this == GetCurrentThreadPointer(kernel));
    ASSERT(timeout > 0);

    ThreadQueueImplForKThreadSleep wait_queue_(kernel);
    {
        // Setup the scheduling lock and sleep.
        KScopedSchedulerLockAndSleep slp(kernel, this, timeout);

        // Check if the thread should terminate.
        if (this->IsTerminationRequested()) {
            slp.CancelSleep();
            return ResultTerminationRequested;
        }

        // Wait for the sleep to end.
        this->BeginWait(std::addressof(wait_queue_));
        SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::Sleep);
    }

    return ResultSuccess;
}

void KThread::IfDummyThreadTryWait() {
    if (!IsDummyThread()) {
        return;
    }

    if (GetState() != ThreadState::Waiting) {
        return;
    }

    // Block until we are no longer waiting.
    std::unique_lock lk(dummy_wait_lock);
    dummy_wait_cv.wait(
        lk, [&] { return GetState() != ThreadState::Waiting || kernel.IsShuttingDown(); });
}

void KThread::IfDummyThreadEndWait() {
    if (!IsDummyThread()) {
        return;
    }

    // Wake up the waiting thread.
    dummy_wait_cv.notify_one();
}

void KThread::BeginWait(KThreadQueue* queue) {
    // Set our state as waiting.
    SetState(ThreadState::Waiting);

    // Set our wait queue.
    wait_queue = queue;
}

void KThread::NotifyAvailable(KSynchronizationObject* signaled_object, ResultCode wait_result_) {
    // Lock the scheduler.
    KScopedSchedulerLock sl(kernel);

    // If we're waiting, notify our queue that we're available.
    if (GetState() == ThreadState::Waiting) {
        wait_queue->NotifyAvailable(this, signaled_object, wait_result_);
    }
}

void KThread::EndWait(ResultCode wait_result_) {
    // Lock the scheduler.
    KScopedSchedulerLock sl(kernel);

    // If we're waiting, notify our queue that we're available.
    if (GetState() == ThreadState::Waiting) {
        if (wait_queue == nullptr) {
            // This should never happen, but avoid a hard crash below to get this logged.
            ASSERT_MSG(false, "wait_queue is nullptr!");
            return;
        }

        wait_queue->EndWait(this, wait_result_);

        // Special case for dummy threads to wakeup if necessary.
        IfDummyThreadEndWait();
    }
}

void KThread::CancelWait(ResultCode wait_result_, bool cancel_timer_task) {
    // Lock the scheduler.
    KScopedSchedulerLock sl(kernel);

    // If we're waiting, notify our queue that we're available.
    if (GetState() == ThreadState::Waiting) {
        wait_queue->CancelWait(this, wait_result_, cancel_timer_task);
    }
}

void KThread::SetState(ThreadState state) {
    KScopedSchedulerLock sl{kernel};

    // Clear debugging state
    SetMutexWaitAddressForDebugging({});
    SetWaitReasonForDebugging({});

    const ThreadState old_state = thread_state.load(std::memory_order_relaxed);
    thread_state.store(
        static_cast<ThreadState>((old_state & ~ThreadState::Mask) | (state & ThreadState::Mask)),
        std::memory_order_relaxed);
    if (thread_state.load(std::memory_order_relaxed) != old_state) {
        KScheduler::OnThreadStateChanged(kernel, this, old_state);
    }
}

std::shared_ptr<Common::Fiber>& KThread::GetHostContext() {
    return host_context;
}

void SetCurrentThread(KernelCore& kernel, KThread* thread) {
    kernel.SetCurrentEmuThread(thread);
}

KThread* GetCurrentThreadPointer(KernelCore& kernel) {
    return kernel.GetCurrentEmuThread();
}

KThread& GetCurrentThread(KernelCore& kernel) {
    return *GetCurrentThreadPointer(kernel);
}

s32 GetCurrentCoreId(KernelCore& kernel) {
    return GetCurrentThread(kernel).GetCurrentCore();
}

KScopedDisableDispatch::~KScopedDisableDispatch() {
    // If we are shutting down the kernel, none of this is relevant anymore.
    if (kernel.IsShuttingDown()) {
        return;
    }

    // Skip the reschedule if single-core, as dispatch tracking is disabled here.
    if (!Settings::values.use_multi_core.GetValue()) {
        return;
    }

    if (GetCurrentThread(kernel).GetDisableDispatchCount() <= 1) {
        auto scheduler = kernel.CurrentScheduler();

        if (scheduler) {
            scheduler->RescheduleCurrentCore();
        }
    } else {
        GetCurrentThread(kernel).EnableDispatch();
    }
}

} // namespace Kernel
