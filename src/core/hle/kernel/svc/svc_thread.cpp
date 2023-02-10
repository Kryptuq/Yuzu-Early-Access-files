// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/scope_exit.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/svc.h"

namespace Kernel::Svc {
namespace {

constexpr bool IsValidVirtualCoreId(int32_t core_id) {
    return (0 <= core_id && core_id < static_cast<int32_t>(Core::Hardware::NUM_CPU_CORES));
}

} // Anonymous namespace

/// Creates a new thread
Result CreateThread(Core::System& system, Handle* out_handle, VAddr entry_point, u64 arg,
                    VAddr stack_bottom, s32 priority, s32 core_id) {
    LOG_DEBUG(Kernel_SVC,
              "called entry_point=0x{:08X}, arg=0x{:08X}, stack_bottom=0x{:08X}, "
              "priority=0x{:08X}, core_id=0x{:08X}",
              entry_point, arg, stack_bottom, priority, core_id);

    // Adjust core id, if it's the default magic.
    auto& kernel = system.Kernel();
    auto& process = *kernel.CurrentProcess();
    if (core_id == IdealCoreUseProcessValue) {
        core_id = process.GetIdealCoreId();
    }

    // Validate arguments.
    if (!IsValidVirtualCoreId(core_id)) {
        LOG_ERROR(Kernel_SVC, "Invalid Core ID specified (id={})", core_id);
        return ResultInvalidCoreId;
    }
    if (((1ULL << core_id) & process.GetCoreMask()) == 0) {
        LOG_ERROR(Kernel_SVC, "Core ID doesn't fall within allowable cores (id={})", core_id);
        return ResultInvalidCoreId;
    }

    if (HighestThreadPriority > priority || priority > LowestThreadPriority) {
        LOG_ERROR(Kernel_SVC, "Invalid priority specified (priority={})", priority);
        return ResultInvalidPriority;
    }
    if (!process.CheckThreadPriority(priority)) {
        LOG_ERROR(Kernel_SVC, "Invalid allowable thread priority (priority={})", priority);
        return ResultInvalidPriority;
    }

    // Reserve a new thread from the process resource limit (waiting up to 100ms).
    KScopedResourceReservation thread_reservation(
        kernel.CurrentProcess(), LimitableResource::ThreadCountMax, 1,
        system.CoreTiming().GetGlobalTimeNs().count() + 100000000);
    if (!thread_reservation.Succeeded()) {
        LOG_ERROR(Kernel_SVC, "Could not reserve a new thread");
        return ResultLimitReached;
    }

    // Create the thread.
    KThread* thread = KThread::Create(kernel);
    if (!thread) {
        LOG_ERROR(Kernel_SVC, "Unable to create new threads. Thread creation limit reached.");
        return ResultOutOfResource;
    }
    SCOPE_EXIT({ thread->Close(); });

    // Initialize the thread.
    {
        KScopedLightLock lk{process.GetStateLock()};
        R_TRY(KThread::InitializeUserThread(system, thread, entry_point, arg, stack_bottom,
                                            priority, core_id, &process));
    }

    // Set the thread name for debugging purposes.
    thread->SetName(fmt::format("thread[entry_point={:X}, handle={:X}]", entry_point, *out_handle));

    // Commit the thread reservation.
    thread_reservation.Commit();

    // Register the new thread.
    KThread::Register(kernel, thread);

    // Add the thread to the handle table.
    R_TRY(process.GetHandleTable().Add(out_handle, thread));

    return ResultSuccess;
}

/// Starts the thread for the provided handle
Result StartThread(Core::System& system, Handle thread_handle) {
    LOG_DEBUG(Kernel_SVC, "called thread=0x{:08X}", thread_handle);

    // Get the thread from its handle.
    KScopedAutoObject thread =
        system.Kernel().CurrentProcess()->GetHandleTable().GetObject<KThread>(thread_handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Try to start the thread.
    R_TRY(thread->Run());

    // If we succeeded, persist a reference to the thread.
    thread->Open();
    system.Kernel().RegisterInUseObject(thread.GetPointerUnsafe());

    return ResultSuccess;
}

/// Called when a thread exits
void ExitThread(Core::System& system) {
    LOG_DEBUG(Kernel_SVC, "called, pc=0x{:08X}", system.CurrentArmInterface().GetPC());

    auto* const current_thread = GetCurrentThreadPointer(system.Kernel());
    system.GlobalSchedulerContext().RemoveThread(current_thread);
    current_thread->Exit();
    system.Kernel().UnregisterInUseObject(current_thread);
}

/// Sleep the current thread
void SleepThread(Core::System& system, s64 nanoseconds) {
    auto& kernel = system.Kernel();
    const auto yield_type = static_cast<Svc::YieldType>(nanoseconds);

    LOG_TRACE(Kernel_SVC, "called nanoseconds={}", nanoseconds);

    // When the input tick is positive, sleep.
    if (nanoseconds > 0) {
        // Convert the timeout from nanoseconds to ticks.
        // NOTE: Nintendo does not use this conversion logic in WaitSynchronization...

        // Sleep.
        // NOTE: Nintendo does not check the result of this sleep.
        static_cast<void>(GetCurrentThread(kernel).Sleep(nanoseconds));
    } else if (yield_type == Svc::YieldType::WithoutCoreMigration) {
        KScheduler::YieldWithoutCoreMigration(kernel);
    } else if (yield_type == Svc::YieldType::WithCoreMigration) {
        KScheduler::YieldWithCoreMigration(kernel);
    } else if (yield_type == Svc::YieldType::ToAnyThread) {
        KScheduler::YieldToAnyThread(kernel);
    } else {
        // Nintendo does nothing at all if an otherwise invalid value is passed.
        ASSERT_MSG(false, "Unimplemented sleep yield type '{:016X}'!", nanoseconds);
    }
}

/// Gets the thread context
Result GetThreadContext3(Core::System& system, VAddr out_context, Handle thread_handle) {
    LOG_DEBUG(Kernel_SVC, "called, out_context=0x{:08X}, thread_handle=0x{:X}", out_context,
              thread_handle);

    auto& kernel = system.Kernel();

    // Get the thread from its handle.
    KScopedAutoObject thread =
        kernel.CurrentProcess()->GetHandleTable().GetObject<KThread>(thread_handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Require the handle be to a non-current thread in the current process.
    const auto* current_process = kernel.CurrentProcess();
    R_UNLESS(current_process == thread->GetOwnerProcess(), ResultInvalidId);

    // Verify that the thread isn't terminated.
    R_UNLESS(thread->GetState() != ThreadState::Terminated, ResultTerminationRequested);

    /// Check that the thread is not the current one.
    /// NOTE: Nintendo does not check this, and thus the following loop will deadlock.
    R_UNLESS(thread.GetPointerUnsafe() != GetCurrentThreadPointer(kernel), ResultInvalidId);

    // Try to get the thread context until the thread isn't current on any core.
    while (true) {
        KScopedSchedulerLock sl{kernel};

        // TODO(bunnei): Enforce that thread is suspended for debug here.

        // If the thread's raw state isn't runnable, check if it's current on some core.
        if (thread->GetRawState() != ThreadState::Runnable) {
            bool current = false;
            for (auto i = 0; i < static_cast<s32>(Core::Hardware::NUM_CPU_CORES); ++i) {
                if (thread.GetPointerUnsafe() == kernel.Scheduler(i).GetSchedulerCurrentThread()) {
                    current = true;
                    break;
                }
            }

            // If the thread is current, retry until it isn't.
            if (current) {
                continue;
            }
        }

        // Get the thread context.
        std::vector<u8> context;
        R_TRY(thread->GetThreadContext3(context));

        // Copy the thread context to user space.
        system.Memory().WriteBlock(out_context, context.data(), context.size());

        return ResultSuccess;
    }

    return ResultSuccess;
}

/// Gets the priority for the specified thread
Result GetThreadPriority(Core::System& system, s32* out_priority, Handle handle) {
    LOG_TRACE(Kernel_SVC, "called");

    // Get the thread from its handle.
    KScopedAutoObject thread =
        system.Kernel().CurrentProcess()->GetHandleTable().GetObject<KThread>(handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Get the thread's priority.
    *out_priority = thread->GetPriority();
    return ResultSuccess;
}

/// Sets the priority for the specified thread
Result SetThreadPriority(Core::System& system, Handle thread_handle, s32 priority) {
    // Get the current process.
    KProcess& process = *system.Kernel().CurrentProcess();

    // Validate the priority.
    R_UNLESS(HighestThreadPriority <= priority && priority <= LowestThreadPriority,
             ResultInvalidPriority);
    R_UNLESS(process.CheckThreadPriority(priority), ResultInvalidPriority);

    // Get the thread from its handle.
    KScopedAutoObject thread = process.GetHandleTable().GetObject<KThread>(thread_handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Set the thread priority.
    thread->SetBasePriority(priority);
    return ResultSuccess;
}

Result GetThreadList(Core::System& system, s32* out_num_threads, VAddr out_thread_ids,
                     s32 out_thread_ids_size, Handle debug_handle) {
    // TODO: Handle this case when debug events are supported.
    UNIMPLEMENTED_IF(debug_handle != InvalidHandle);

    LOG_DEBUG(Kernel_SVC, "called. out_thread_ids=0x{:016X}, out_thread_ids_size={}",
              out_thread_ids, out_thread_ids_size);

    // If the size is negative or larger than INT32_MAX / sizeof(u64)
    if ((out_thread_ids_size & 0xF0000000) != 0) {
        LOG_ERROR(Kernel_SVC, "Supplied size outside [0, 0x0FFFFFFF] range. size={}",
                  out_thread_ids_size);
        return ResultOutOfRange;
    }

    auto* const current_process = system.Kernel().CurrentProcess();
    const auto total_copy_size = out_thread_ids_size * sizeof(u64);

    if (out_thread_ids_size > 0 &&
        !current_process->PageTable().IsInsideAddressSpace(out_thread_ids, total_copy_size)) {
        LOG_ERROR(Kernel_SVC, "Address range outside address space. begin=0x{:016X}, end=0x{:016X}",
                  out_thread_ids, out_thread_ids + total_copy_size);
        return ResultInvalidCurrentMemory;
    }

    auto& memory = system.Memory();
    const auto& thread_list = current_process->GetThreadList();
    const auto num_threads = thread_list.size();
    const auto copy_amount = std::min(static_cast<std::size_t>(out_thread_ids_size), num_threads);

    auto list_iter = thread_list.cbegin();
    for (std::size_t i = 0; i < copy_amount; ++i, ++list_iter) {
        memory.Write64(out_thread_ids, (*list_iter)->GetThreadID());
        out_thread_ids += sizeof(u64);
    }

    *out_num_threads = static_cast<u32>(num_threads);
    return ResultSuccess;
}

Result GetThreadCoreMask(Core::System& system, s32* out_core_id, u64* out_affinity_mask,
                         Handle thread_handle) {
    LOG_TRACE(Kernel_SVC, "called, handle=0x{:08X}", thread_handle);

    // Get the thread from its handle.
    KScopedAutoObject thread =
        system.Kernel().CurrentProcess()->GetHandleTable().GetObject<KThread>(thread_handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Get the core mask.
    R_TRY(thread->GetCoreMask(out_core_id, out_affinity_mask));

    return ResultSuccess;
}

Result SetThreadCoreMask(Core::System& system, Handle thread_handle, s32 core_id,
                         u64 affinity_mask) {
    // Determine the core id/affinity mask.
    if (core_id == IdealCoreUseProcessValue) {
        core_id = system.Kernel().CurrentProcess()->GetIdealCoreId();
        affinity_mask = (1ULL << core_id);
    } else {
        // Validate the affinity mask.
        const u64 process_core_mask = system.Kernel().CurrentProcess()->GetCoreMask();
        R_UNLESS((affinity_mask | process_core_mask) == process_core_mask, ResultInvalidCoreId);
        R_UNLESS(affinity_mask != 0, ResultInvalidCombination);

        // Validate the core id.
        if (IsValidVirtualCoreId(core_id)) {
            R_UNLESS(((1ULL << core_id) & affinity_mask) != 0, ResultInvalidCombination);
        } else {
            R_UNLESS(core_id == IdealCoreNoUpdate || core_id == IdealCoreDontCare,
                     ResultInvalidCoreId);
        }
    }

    // Get the thread from its handle.
    KScopedAutoObject thread =
        system.Kernel().CurrentProcess()->GetHandleTable().GetObject<KThread>(thread_handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Set the core mask.
    R_TRY(thread->SetCoreMask(core_id, affinity_mask));

    return ResultSuccess;
}

/// Get the ID for the specified thread.
Result GetThreadId(Core::System& system, u64* out_thread_id, Handle thread_handle) {
    // Get the thread from its handle.
    KScopedAutoObject thread =
        system.Kernel().CurrentProcess()->GetHandleTable().GetObject<KThread>(thread_handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Get the thread's id.
    *out_thread_id = thread->GetId();
    return ResultSuccess;
}

Result CreateThread64(Core::System& system, Handle* out_handle, uint64_t func, uint64_t arg,
                      uint64_t stack_bottom, int32_t priority, int32_t core_id) {
    R_RETURN(CreateThread(system, out_handle, func, arg, stack_bottom, priority, core_id));
}

Result StartThread64(Core::System& system, Handle thread_handle) {
    R_RETURN(StartThread(system, thread_handle));
}

void ExitThread64(Core::System& system) {
    return ExitThread(system);
}

void SleepThread64(Core::System& system, int64_t ns) {
    return SleepThread(system, ns);
}

Result GetThreadPriority64(Core::System& system, int32_t* out_priority, Handle thread_handle) {
    R_RETURN(GetThreadPriority(system, out_priority, thread_handle));
}

Result SetThreadPriority64(Core::System& system, Handle thread_handle, int32_t priority) {
    R_RETURN(SetThreadPriority(system, thread_handle, priority));
}

Result GetThreadCoreMask64(Core::System& system, int32_t* out_core_id, uint64_t* out_affinity_mask,
                           Handle thread_handle) {
    R_RETURN(GetThreadCoreMask(system, out_core_id, out_affinity_mask, thread_handle));
}

Result SetThreadCoreMask64(Core::System& system, Handle thread_handle, int32_t core_id,
                           uint64_t affinity_mask) {
    R_RETURN(SetThreadCoreMask(system, thread_handle, core_id, affinity_mask));
}

Result GetThreadId64(Core::System& system, uint64_t* out_thread_id, Handle thread_handle) {
    R_RETURN(GetThreadId(system, out_thread_id, thread_handle));
}

Result GetThreadContext364(Core::System& system, uint64_t out_context, Handle thread_handle) {
    R_RETURN(GetThreadContext3(system, out_context, thread_handle));
}

Result GetThreadList64(Core::System& system, int32_t* out_num_threads, uint64_t out_thread_ids,
                       int32_t max_out_count, Handle debug_handle) {
    R_RETURN(GetThreadList(system, out_num_threads, out_thread_ids, max_out_count, debug_handle));
}

Result CreateThread64From32(Core::System& system, Handle* out_handle, uint32_t func, uint32_t arg,
                            uint32_t stack_bottom, int32_t priority, int32_t core_id) {
    R_RETURN(CreateThread(system, out_handle, func, arg, stack_bottom, priority, core_id));
}

Result StartThread64From32(Core::System& system, Handle thread_handle) {
    R_RETURN(StartThread(system, thread_handle));
}

void ExitThread64From32(Core::System& system) {
    return ExitThread(system);
}

void SleepThread64From32(Core::System& system, int64_t ns) {
    return SleepThread(system, ns);
}

Result GetThreadPriority64From32(Core::System& system, int32_t* out_priority,
                                 Handle thread_handle) {
    R_RETURN(GetThreadPriority(system, out_priority, thread_handle));
}

Result SetThreadPriority64From32(Core::System& system, Handle thread_handle, int32_t priority) {
    R_RETURN(SetThreadPriority(system, thread_handle, priority));
}

Result GetThreadCoreMask64From32(Core::System& system, int32_t* out_core_id,
                                 uint64_t* out_affinity_mask, Handle thread_handle) {
    R_RETURN(GetThreadCoreMask(system, out_core_id, out_affinity_mask, thread_handle));
}

Result SetThreadCoreMask64From32(Core::System& system, Handle thread_handle, int32_t core_id,
                                 uint64_t affinity_mask) {
    R_RETURN(SetThreadCoreMask(system, thread_handle, core_id, affinity_mask));
}

Result GetThreadId64From32(Core::System& system, uint64_t* out_thread_id, Handle thread_handle) {
    R_RETURN(GetThreadId(system, out_thread_id, thread_handle));
}

Result GetThreadContext364From32(Core::System& system, uint32_t out_context, Handle thread_handle) {
    R_RETURN(GetThreadContext3(system, out_context, thread_handle));
}

Result GetThreadList64From32(Core::System& system, int32_t* out_num_threads,
                             uint32_t out_thread_ids, int32_t max_out_count, Handle debug_handle) {
    R_RETURN(GetThreadList(system, out_num_threads, out_thread_ids, max_out_count, debug_handle));
}

} // namespace Kernel::Svc
