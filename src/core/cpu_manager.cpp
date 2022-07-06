// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/fiber.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu_manager.h"
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
    running_mode = true;
    num_cores = is_multicore ? Core::Hardware::NUM_CPU_CORES : 1;
    gpu_barrier = std::make_unique<Common::Barrier>(num_cores + 1);
    pause_barrier = std::make_unique<Common::Barrier>(num_cores + 1);

    for (std::size_t core = 0; core < num_cores; core++) {
        core_data[core].host_thread = std::jthread(ThreadStart, std::ref(*this), core);
    }
}

void CpuManager::Shutdown() {
    running_mode = false;
    Pause(false);
}

void CpuManager::GuestThreadFunction() {
    if (is_multicore) {
        MultiCoreRunGuestThread();
    } else {
        SingleCoreRunGuestThread();
    }
}

void CpuManager::GuestRewindFunction() {
    if (is_multicore) {
        MultiCoreRunGuestLoop();
    } else {
        SingleCoreRunGuestLoop();
    }
}

void CpuManager::IdleThreadFunction() {
    if (is_multicore) {
        MultiCoreRunIdleThread();
    } else {
        SingleCoreRunIdleThread();
    }
}

///////////////////////////////////////////////////////////////////////////////
///                             MultiCore                                   ///
///////////////////////////////////////////////////////////////////////////////

void CpuManager::MultiCoreRunGuestThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    auto* thread = kernel.CurrentScheduler()->GetSchedulerCurrentThread();
    auto& host_context = thread->GetHostContext();
    host_context->SetRewindPoint([this] { GuestRewindFunction(); });
    MultiCoreRunGuestLoop();
}

void CpuManager::MultiCoreRunGuestLoop() {
    auto& kernel = system.Kernel();

    while (true) {
        auto* physical_core = &kernel.CurrentPhysicalCore();
        while (!physical_core->IsInterrupted()) {
            physical_core->Run();
            physical_core = &kernel.CurrentPhysicalCore();
        }
        {
            Kernel::KScopedDisableDispatch dd(kernel);
            physical_core->ArmInterface().ClearExclusiveState();
        }
    }
}

void CpuManager::MultiCoreRunIdleThread() {
    auto& kernel = system.Kernel();
    while (true) {
        Kernel::KScopedDisableDispatch dd(kernel);
        kernel.CurrentPhysicalCore().Idle();
    }
}

///////////////////////////////////////////////////////////////////////////////
///                             SingleCore                                   ///
///////////////////////////////////////////////////////////////////////////////

void CpuManager::SingleCoreRunGuestThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    auto* thread = kernel.CurrentScheduler()->GetSchedulerCurrentThread();
    auto& host_context = thread->GetHostContext();
    host_context->SetRewindPoint([this] { GuestRewindFunction(); });
    SingleCoreRunGuestLoop();
}

void CpuManager::SingleCoreRunGuestLoop() {
    auto& kernel = system.Kernel();
    while (true) {
        auto* physical_core = &kernel.CurrentPhysicalCore();
        if (!physical_core->IsInterrupted()) {
            physical_core->Run();
            physical_core = &kernel.CurrentPhysicalCore();
        }
        kernel.SetIsPhantomModeForSingleCore(true);
        system.CoreTiming().Advance();
        kernel.SetIsPhantomModeForSingleCore(false);
        physical_core->ArmInterface().ClearExclusiveState();
        PreemptSingleCore();
        auto& scheduler = kernel.Scheduler(current_core);
        scheduler.RescheduleCurrentCore();
    }
}

void CpuManager::SingleCoreRunIdleThread() {
    auto& kernel = system.Kernel();
    while (true) {
        auto& physical_core = kernel.CurrentPhysicalCore();
        PreemptSingleCore(false);
        system.CoreTiming().AddTicks(1000U);
        idle_count++;
        auto& scheduler = physical_core.Scheduler();
        scheduler.RescheduleCurrentCore();
    }
}

void CpuManager::PreemptSingleCore(bool from_running_enviroment) {
    {
        auto& kernel = system.Kernel();
        auto& scheduler = kernel.Scheduler(current_core);
        Kernel::KThread* current_thread = scheduler.GetSchedulerCurrentThread();
        if (idle_count >= 4 || from_running_enviroment) {
            if (!from_running_enviroment) {
                system.CoreTiming().Idle();
                idle_count = 0;
            }
            kernel.SetIsPhantomModeForSingleCore(true);
            system.CoreTiming().Advance();
            kernel.SetIsPhantomModeForSingleCore(false);
        }
        current_core.store((current_core + 1) % Core::Hardware::NUM_CPU_CORES);
        system.CoreTiming().ResetTicks();
        scheduler.Unload(scheduler.GetSchedulerCurrentThread());

        auto& next_scheduler = kernel.Scheduler(current_core);
        Common::Fiber::YieldTo(current_thread->GetHostContext(), *next_scheduler.ControlContext());
    }

    // May have changed scheduler
    {
        auto& scheduler = system.Kernel().Scheduler(current_core);
        scheduler.Reload(scheduler.GetSchedulerCurrentThread());
        if (!scheduler.IsIdle()) {
            idle_count = 0;
        }
    }
}

void CpuManager::SuspendThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();

    while (true) {
        auto core = is_multicore ? kernel.CurrentPhysicalCoreIndex() : 0;
        auto& scheduler = *kernel.CurrentScheduler();
        Kernel::KThread* current_thread = scheduler.GetSchedulerCurrentThread();
        Common::Fiber::YieldTo(current_thread->GetHostContext(), *core_data[core].host_context);

        // This shouldn't be here. This is here because the scheduler needs the current
        // thread to have dispatch disabled before explicitly rescheduling. Ideally in the
        // future this will be called by RequestScheduleOnInterrupt and explicitly disabling
        // dispatch outside the scheduler will not be necessary.
        current_thread->DisableDispatch();

        scheduler.RescheduleCurrentCore();
    }
}

void CpuManager::Pause(bool paused) {
    std::scoped_lock lk{pause_lock};

    if (pause_state == paused) {
        return;
    }

    // Set the new state
    pause_state.store(paused);

    // Wake up any waiting threads
    pause_state.notify_all();

    // Wait for all threads to successfully change state before returning
    pause_barrier->Sync();
}

void CpuManager::RunThread(std::size_t core) {
    /// Initialization
    system.RegisterCoreThread(core);
    std::string name;
    if (is_multicore) {
        name = "yuzu:CPUCore_" + std::to_string(core);
    } else {
        name = "yuzu:CPUThread";
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

    {
        // Set the current thread on entry
        auto* current_thread = system.Kernel().CurrentScheduler()->GetIdleThread();
        Kernel::SetCurrentThread(system.Kernel(), current_thread);
    }

    while (running_mode) {
        if (pause_state.load(std::memory_order_relaxed)) {
            // Wait for caller to acknowledge pausing
            pause_barrier->Sync();

            // Wait until unpaused
            pause_state.wait(true, std::memory_order_relaxed);

            // Wait for caller to acknowledge unpausing
            pause_barrier->Sync();
        }

        auto current_thread = system.Kernel().CurrentScheduler()->GetSchedulerCurrentThread();
        Common::Fiber::YieldTo(data.host_context, *current_thread->GetHostContext());
    }
}

} // namespace Core
