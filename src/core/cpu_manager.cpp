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

CpuManager::CpuManager(System& system_)
    : pause_barrier{std::make_unique<Common::Barrier>(1)}, system{system_} {}
CpuManager::~CpuManager() = default;

void CpuManager::ThreadStart(std::stop_token stop_token, CpuManager& cpu_manager,
                             std::size_t core) {
    cpu_manager.RunThread(stop_token, core);
}

void CpuManager::Initialize() {
    running_mode = true;
    if (is_multicore) {
        for (std::size_t core = 0; core < Core::Hardware::NUM_CPU_CORES; core++) {
            core_data[core].host_thread = std::jthread(ThreadStart, std::ref(*this), core);
        }
        pause_barrier = std::make_unique<Common::Barrier>(Core::Hardware::NUM_CPU_CORES + 1);
    } else {
        core_data[0].host_thread = std::jthread(ThreadStart, std::ref(*this), 0);
        pause_barrier = std::make_unique<Common::Barrier>(2);
    }
}

void CpuManager::Shutdown() {
    running_mode = false;
    Pause(false);
}

std::function<void(void*)> CpuManager::GetGuestThreadStartFunc() {
    return GuestThreadFunction;
}

std::function<void(void*)> CpuManager::GetIdleThreadStartFunc() {
    return IdleThreadFunction;
}

std::function<void(void*)> CpuManager::GetSuspendThreadStartFunc() {
    return SuspendThreadFunction;
}

void CpuManager::GuestThreadFunction(void* cpu_manager_) {
    CpuManager* cpu_manager = static_cast<CpuManager*>(cpu_manager_);
    if (cpu_manager->is_multicore) {
        cpu_manager->MultiCoreRunGuestThread();
    } else {
        cpu_manager->SingleCoreRunGuestThread();
    }
}

void CpuManager::GuestRewindFunction(void* cpu_manager_) {
    CpuManager* cpu_manager = static_cast<CpuManager*>(cpu_manager_);
    if (cpu_manager->is_multicore) {
        cpu_manager->MultiCoreRunGuestLoop();
    } else {
        cpu_manager->SingleCoreRunGuestLoop();
    }
}

void CpuManager::IdleThreadFunction(void* cpu_manager_) {
    CpuManager* cpu_manager = static_cast<CpuManager*>(cpu_manager_);
    if (cpu_manager->is_multicore) {
        cpu_manager->MultiCoreRunIdleThread();
    } else {
        cpu_manager->SingleCoreRunIdleThread();
    }
}

void CpuManager::SuspendThreadFunction(void* cpu_manager_) {
    CpuManager* cpu_manager = static_cast<CpuManager*>(cpu_manager_);
    if (cpu_manager->is_multicore) {
        cpu_manager->MultiCoreRunSuspendThread();
    } else {
        cpu_manager->SingleCoreRunSuspendThread();
    }
}

void* CpuManager::GetStartFuncParamater() {
    return static_cast<void*>(this);
}

///////////////////////////////////////////////////////////////////////////////
///                             MultiCore                                   ///
///////////////////////////////////////////////////////////////////////////////

void CpuManager::MultiCoreRunGuestThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    auto* thread = kernel.CurrentScheduler()->GetCurrentThread();
    auto& host_context = thread->GetHostContext();
    host_context->SetRewindPoint(GuestRewindFunction, this);
    MultiCoreRunGuestLoop();
}

void CpuManager::MultiCoreRunGuestLoop() {
    auto& kernel = system.Kernel();

    while (true) {
        auto* physical_core = &kernel.CurrentPhysicalCore();
        system.EnterDynarmicProfile();
        while (!physical_core->IsInterrupted()) {
            physical_core->Run();
            physical_core = &kernel.CurrentPhysicalCore();
        }
        system.ExitDynarmicProfile();
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

void CpuManager::MultiCoreRunSuspendThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    while (true) {
        auto core = kernel.CurrentPhysicalCoreIndex();
        auto& scheduler = *kernel.CurrentScheduler();
        Kernel::KThread* current_thread = scheduler.GetCurrentThread();
        Common::Fiber::YieldTo(current_thread->GetHostContext(), *core_data[core].host_context);
        ASSERT(scheduler.ContextSwitchPending());
        ASSERT(core == kernel.CurrentPhysicalCoreIndex());
        scheduler.RescheduleCurrentCore();
    }
}

///////////////////////////////////////////////////////////////////////////////
///                             SingleCore                                   ///
///////////////////////////////////////////////////////////////////////////////

void CpuManager::SingleCoreRunGuestThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    auto* thread = kernel.CurrentScheduler()->GetCurrentThread();
    auto& host_context = thread->GetHostContext();
    host_context->SetRewindPoint(GuestRewindFunction, this);
    SingleCoreRunGuestLoop();
}

void CpuManager::SingleCoreRunGuestLoop() {
    auto& kernel = system.Kernel();
    while (true) {
        auto* physical_core = &kernel.CurrentPhysicalCore();
        system.EnterDynarmicProfile();
        if (!physical_core->IsInterrupted()) {
            physical_core->Run();
            physical_core = &kernel.CurrentPhysicalCore();
        }
        system.ExitDynarmicProfile();
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

void CpuManager::SingleCoreRunSuspendThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    while (true) {
        auto core = kernel.GetCurrentHostThreadID();
        auto& scheduler = *kernel.CurrentScheduler();
        Kernel::KThread* current_thread = scheduler.GetCurrentThread();
        Common::Fiber::YieldTo(current_thread->GetHostContext(), *core_data[0].host_context);
        ASSERT(scheduler.ContextSwitchPending());
        ASSERT(core == kernel.GetCurrentHostThreadID());
        scheduler.RescheduleCurrentCore();
    }
}

void CpuManager::PreemptSingleCore(bool from_running_enviroment) {
    {
        auto& kernel = system.Kernel();
        auto& scheduler = kernel.Scheduler(current_core);
        Kernel::KThread* current_thread = scheduler.GetCurrentThread();
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
        scheduler.Unload(scheduler.GetCurrentThread());

        auto& next_scheduler = kernel.Scheduler(current_core);
        Common::Fiber::YieldTo(current_thread->GetHostContext(), *next_scheduler.ControlContext());
    }

    // May have changed scheduler
    {
        auto& scheduler = system.Kernel().Scheduler(current_core);
        scheduler.Reload(scheduler.GetCurrentThread());
        if (!scheduler.IsIdle()) {
            idle_count = 0;
        }
    }
}

void CpuManager::Pause(bool paused) {
    if (pause_state == paused) {
        return;
    }

    pause_state.store(paused);
    pause_state.notify_all();

    // Wait for all threads to successfully pause/unpause before returning
    pause_barrier->Sync();
}

void CpuManager::RunThread(std::stop_token stop_token, std::size_t core) {
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
    const bool sc_sync = !is_async_gpu && !is_multicore;
    bool sc_sync_first_use = sc_sync;

    // Cleanup
    SCOPE_EXIT({
        data.host_context->Exit();
        MicroProfileOnThreadExit();
    });

    /// Running
    while (running_mode) {
        if (pause_state.load(std::memory_order_relaxed)) {
            // Wait for caller to acknowledge pausing
            pause_barrier->Sync();

            // Wait until unpaused
            pause_state.wait(true, std::memory_order_relaxed);

            // Wait for caller to acknowledge unpausing
            pause_barrier->Sync();
        }

        if (sc_sync_first_use) {
            system.GPU().ObtainContext();
            sc_sync_first_use = false;
        }

        // Emulation was stopped
        if (stop_token.stop_requested()) {
            return;
        }

        auto current_thread = system.Kernel().CurrentScheduler()->GetCurrentThread();
        Common::Fiber::YieldTo(data.host_context, *current_thread->GetHostContext());
    }
}

} // namespace Core
