// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/k_interrupt_manager.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel::KInterruptManager {

void HandleInterrupt(KernelCore& kernel, s32 core_id) {
    auto* process = kernel.CurrentProcess();
    if (!process) {
        return;
    }

    auto& scheduler = kernel.Scheduler(core_id);
    auto& current_thread = *scheduler.GetCurrentThread();

    // If the user disable count is set, we may need to pin the current thread.
    if (current_thread.GetUserDisableCount() && !process->GetPinnedThread(core_id)) {
        KScopedSchedulerLock sl{kernel};

        // Pin the current thread.
        process->PinCurrentThread(core_id);

        // Set the interrupt flag for the thread.
        scheduler.GetCurrentThread()->SetInterruptFlag();
    }
}

} // namespace Kernel::KInterruptManager
