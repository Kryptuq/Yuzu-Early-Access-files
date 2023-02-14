// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_interrupt_manager.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel::KInterruptManager {

void HandleInterrupt(KernelCore& kernel, s32 core_id) {
    // Acknowledge the interrupt.
    kernel.PhysicalCore(core_id).ClearInterrupt();

    auto& current_thread = GetCurrentThread(kernel);

    if (auto* process = GetCurrentProcessPointer(kernel); process) {
        // If the user disable count is set, we may need to pin the current thread.
        if (current_thread.GetUserDisableCount() && !process->GetPinnedThread(core_id)) {
            KScopedSchedulerLock sl{kernel};

            // Pin the current thread.
            process->PinCurrentThread(core_id);

            // Set the interrupt flag for the thread.
            GetCurrentThread(kernel).SetInterruptFlag();
        }
    }

    // Request interrupt scheduling.
    kernel.CurrentScheduler()->RequestScheduleOnInterrupt();
}

void SendInterProcessorInterrupt(KernelCore& kernel, u64 core_mask) {
    for (std::size_t core_id = 0; core_id < Core::Hardware::NUM_CPU_CORES; ++core_id) {
        if (core_mask & (1ULL << core_id)) {
            kernel.PhysicalCore(core_id).Interrupt();
        }
    }
}

} // namespace Kernel::KInterruptManager
