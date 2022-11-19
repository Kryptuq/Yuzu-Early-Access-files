// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/fiber.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu_manager.h"
#include "core/hle/kernel/k_interrupt_manager.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"
#include "video_core/gpu.h"

namespace Core {

CpuManager::CpuManager(System& system_) : system{system_} {}
CpuManager::~CpuManager() = default;

void CpuManager::ThreadStart(std::stop_token stop_token, CpuManager& cpu_manager,
                             std::size_t core) {
    cpu_manager.RunThread(core);
}

void CpuManager::Initialize() {
    num_cores = is_multicore ? Core::Hardware::NUM_CPU_CORES : 1;
    gpu_barrier = std::make_unique<Common::Barrier>(num_cores + 1);

    for (std::size_t core = 0; core < num_cores; core++) {
        core_data[core].host_thread = std::jthread(ThreadStart, std::ref(*this), core);
    }
}

void CpuManager::Shutdown() {
    for (std::size_t core = 0; core < num_cores; core++) {
        if (core_data[core].host_thread.joinable()) {
            core_data[core].host_thread.join();
        }
    }
}

void CpuManager::GuestThreadFunction() {
    if (is_multicore) {
        MultiCoreRunGuestThread();
    } else {
        SingleCoreRunGuestThread();
    }
}

void CpuManager::IdleThreadFunction() {
    if (is_multicore) {
        MultiCoreRunIdleThread();
    } else {
        SingleCoreRunIdleThread();
    }
}

void CpuManager::ShutdownThreadFunction() {
    ShutdownThread();
}

void CpuManager::HandleInterrupt() {
    auto& kernel = system.Kernel();
    auto core_index = kernel.CurrentPhysicalCoreIndex();

    Kernel::KInterruptManager::HandleInterrupt(kernel, static_cast<s32>(core_index));
}

///////////////////////////////////////////////////////////////////////////////
///                             MultiCore                                   ///
///////////////////////////////////////////////////////////////////////////////

void CpuManager::MultiCoreRunGuestThread() {
    // Similar to UserModeThreadStarter in HOS
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();

    while (true) {
        auto* physical_core = &kernel.CurrentPhysicalCore();
        while (!physical_core->IsInterrupted()) {
            physical_core->Run();
            physical_core = &kernel.CurrentPhysicalCore();
        }

        HandleInterrupt();
    }
}

void CpuManager::MultiCoreRunIdleThread() {
    // Not accurate to HOS. Remove this entire method when singlecore is removed.
    // See notes in KScheduler::ScheduleImpl for more information about why this
    // is inaccurate.

    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();

    while (true) {
        auto& physical_core = kernel.CurrentPhysicalCore();
        if (!physical_core.IsInterrupted()) {
            physical_core.Idle();
        }

        HandleInterrupt();
    }
}

///////////////////////////////////////////////////////////////////////////////
///                             SingleCore                                   ///
///////////////////////////////////////////////////////////////////////////////

void CpuManager::SingleCoreRunGuestThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();

    while (true) {
        auto* physical_core = &kernel.CurrentPhysicalCore();
        if (!physical_core->IsInterrupted()) {
            physical_core->Run();
            physical_core = &kernel.CurrentPhysicalCore();
        }

        kernel.SetIsPhantomModeForSingleCore(true);
        system.CoreTiming().Advance();
        kernel.SetIsPhantomModeForSingleCore(false);

        PreemptSingleCore();
        HandleInterrupt();
    }
}

void CpuManager::SingleCoreRunIdleThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();

    while (true) {
        PreemptSingleCore(false);
        system.CoreTiming().AddTicks(1000U);
        idle_count++;
        HandleInterrupt();
    }
}

void CpuManager::PreemptSingleCore(bool from_running_environment) {
    auto& kernel = system.Kernel();

    if (idle_count >= 4 || from_running_environment) {
        if (!from_running_environment) {
            system.CoreTiming().Idle();
            idle_count = 0;
        }
        kernel.SetIsPhantomModeForSingleCore(true);
        system.CoreTiming().Advance();
        kernel.SetIsPhantomModeForSingleCore(false);
    }
    current_core.store((current_core + 1) % Core::Hardware::NUM_CPU_CORES);
    system.CoreTiming().ResetTicks();
    kernel.Scheduler(current_core).PreemptSingleCore();

    // We've now been scheduled again, and we may have exchanged schedulers.
    // Reload the scheduler in case it's different.
    if (!kernel.Scheduler(current_core).IsIdle()) {
        idle_count = 0;
    }
}

void CpuManager::GuestActivate() {
    // Similar to the HorizonKernelMain callback in HOS
    auto& kernel = system.Kernel();
    auto* scheduler = kernel.CurrentScheduler();

    scheduler->Activate();
    UNREACHABLE();
}

void CpuManager::ShutdownThread() {
    auto& kernel = system.Kernel();
    auto* thread = kernel.GetCurrentEmuThread();
    auto core = is_multicore ? kernel.CurrentPhysicalCoreIndex() : 0;

    Common::Fiber::YieldTo(thread->GetHostContext(), core_data[core].host_context);
    UNREACHABLE();
}

void CpuManager::RunThread(std::size_t core) {
    /// Initialization
    system.RegisterCoreThread(core);
    std::string name;
    if (is_multicore) {
        name = "CPUCore_" + std::to_string(core);
    } else {
        name = "CPUThread";
    }
    MicroProfileOnThreadCreate(name.c_str());
    Common::SetCurrentThreadName(name.c_str());
    Common::SetCurrentThreadPriority(Common::ThreadPriority::High);
    auto& data = core_data[core];
    data.host_context = Common::Fiber::ThreadToFiber();

    // Cleanup
    SCOPE_EXIT({
        data.host_context->Exit();
        MicroProfileOnThreadExit();
    });

    // Running
    gpu_barrier->Sync();

    if (!is_async_gpu && !is_multicore) {
        system.GPU().ObtainContext();
    }

    auto& kernel = system.Kernel();
    auto& scheduler = *kernel.CurrentScheduler();
    auto* thread = scheduler.GetSchedulerCurrentThread();
    Kernel::SetCurrentThread(kernel, thread);

    Common::Fiber::YieldTo(data.host_context, thread->GetHostContext());
}

} // namespace Core
