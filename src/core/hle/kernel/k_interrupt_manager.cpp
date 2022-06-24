// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

    auto& current_thread = GetCurrentThread(kernel);

    // If the user disable count is set, we may need to pin the current thread.
    if (current_thread.GetUserDisableCount() && !process->GetPinnedThread(core_id)) {
        KScopedSchedulerLock sl{kernel};

        // Pin the current thread.
        process->PinCurrentThread(core_id);

        // Set the interrupt flag for the thread.
        GetCurrentThread(kernel).SetInterruptFlag();
    }
}

} // namespace Kernel::KInterruptManager
