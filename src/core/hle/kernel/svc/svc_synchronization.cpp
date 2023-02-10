// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/scope_exit.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/svc.h"

namespace Kernel::Svc {

/// Close a handle
Result CloseHandle(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "Closing handle 0x{:08X}", handle);

    // Remove the handle.
    R_UNLESS(system.Kernel().CurrentProcess()->GetHandleTable().Remove(handle),
             ResultInvalidHandle);

    return ResultSuccess;
}

/// Clears the signaled state of an event or process.
Result ResetSignal(Core::System& system, Handle handle) {
    LOG_DEBUG(Kernel_SVC, "called handle 0x{:08X}", handle);

    // Get the current handle table.
    const auto& handle_table = system.Kernel().CurrentProcess()->GetHandleTable();

    // Try to reset as readable event.
    {
        KScopedAutoObject readable_event = handle_table.GetObject<KReadableEvent>(handle);
        if (readable_event.IsNotNull()) {
            return readable_event->Reset();
        }
    }

    // Try to reset as process.
    {
        KScopedAutoObject process = handle_table.GetObject<KProcess>(handle);
        if (process.IsNotNull()) {
            return process->Reset();
        }
    }

    LOG_ERROR(Kernel_SVC, "invalid handle (0x{:08X})", handle);

    return ResultInvalidHandle;
}

/// Wait for the given handles to synchronize, timeout after the specified nanoseconds
Result WaitSynchronization(Core::System& system, s32* index, VAddr handles_address, s32 num_handles,
                           s64 nano_seconds) {
    LOG_TRACE(Kernel_SVC, "called handles_address=0x{:X}, num_handles={}, nano_seconds={}",
              handles_address, num_handles, nano_seconds);

    // Ensure number of handles is valid.
    R_UNLESS(0 <= num_handles && num_handles <= ArgumentHandleCountMax, ResultOutOfRange);

    auto& kernel = system.Kernel();
    std::vector<KSynchronizationObject*> objs(num_handles);
    const auto& handle_table = kernel.CurrentProcess()->GetHandleTable();
    Handle* handles = system.Memory().GetPointer<Handle>(handles_address);

    // Copy user handles.
    if (num_handles > 0) {
        // Convert the handles to objects.
        R_UNLESS(handle_table.GetMultipleObjects<KSynchronizationObject>(objs.data(), handles,
                                                                         num_handles),
                 ResultInvalidHandle);
        for (const auto& obj : objs) {
            kernel.RegisterInUseObject(obj);
        }
    }

    // Ensure handles are closed when we're done.
    SCOPE_EXIT({
        for (s32 i = 0; i < num_handles; ++i) {
            kernel.UnregisterInUseObject(objs[i]);
            objs[i]->Close();
        }
    });

    return KSynchronizationObject::Wait(kernel, index, objs.data(), static_cast<s32>(objs.size()),
                                        nano_seconds);
}

/// Resumes a thread waiting on WaitSynchronization
Result CancelSynchronization(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "called handle=0x{:X}", handle);

    // Get the thread from its handle.
    KScopedAutoObject thread =
        system.Kernel().CurrentProcess()->GetHandleTable().GetObject<KThread>(handle);
    R_UNLESS(thread.IsNotNull(), ResultInvalidHandle);

    // Cancel the thread's wait.
    thread->WaitCancel();
    return ResultSuccess;
}

void SynchronizePreemptionState(Core::System& system) {
    auto& kernel = system.Kernel();

    // Lock the scheduler.
    KScopedSchedulerLock sl{kernel};

    // If the current thread is pinned, unpin it.
    KProcess* cur_process = system.Kernel().CurrentProcess();
    const auto core_id = GetCurrentCoreId(kernel);

    if (cur_process->GetPinnedThread(core_id) == GetCurrentThreadPointer(kernel)) {
        // Clear the current thread's interrupt flag.
        GetCurrentThread(kernel).ClearInterruptFlag();

        // Unpin the current thread.
        cur_process->UnpinCurrentThread(core_id);
    }
}

Result CloseHandle64(Core::System& system, Handle handle) {
    R_RETURN(CloseHandle(system, handle));
}

Result ResetSignal64(Core::System& system, Handle handle) {
    R_RETURN(ResetSignal(system, handle));
}

Result WaitSynchronization64(Core::System& system, int32_t* out_index, uint64_t handles,
                             int32_t num_handles, int64_t timeout_ns) {
    R_RETURN(WaitSynchronization(system, out_index, handles, num_handles, timeout_ns));
}

Result CancelSynchronization64(Core::System& system, Handle handle) {
    R_RETURN(CancelSynchronization(system, handle));
}

void SynchronizePreemptionState64(Core::System& system) {
    SynchronizePreemptionState(system);
}

Result CloseHandle64From32(Core::System& system, Handle handle) {
    R_RETURN(CloseHandle(system, handle));
}

Result ResetSignal64From32(Core::System& system, Handle handle) {
    R_RETURN(ResetSignal(system, handle));
}

Result WaitSynchronization64From32(Core::System& system, int32_t* out_index, uint32_t handles,
                                   int32_t num_handles, int64_t timeout_ns) {
    R_RETURN(WaitSynchronization(system, out_index, handles, num_handles, timeout_ns));
}

Result CancelSynchronization64From32(Core::System& system, Handle handle) {
    R_RETURN(CancelSynchronization(system, handle));
}

void SynchronizePreemptionState64From32(Core::System& system) {
    SynchronizePreemptionState(system);
}

} // namespace Kernel::Svc
