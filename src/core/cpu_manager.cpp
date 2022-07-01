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

std::function<void(void*)> CpuManager::GetGuestThreadStartFunc() {
    return GuestThreadFunction;
}

std::function<void(void*)> CpuManager::GetIdleThreadStartFunc() {
    return IdleThreadFunction;
}

std::function<void(void*)> CpuManager::GetShutdownThreadStartFunc() {
    return ShutdownThreadFunction;
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

void CpuManager::ShutdownThreadFunction(void* cpu_manager) {
    static_cast<CpuManager*>(cpu_manager)->ShutdownThread();
}

void* CpuManager::GetStartFuncParameter() {
    return this;
}

///////////////////////////////////////////////////////////////////////////////
///                             MultiCore                                   ///
///////////////////////////////////////////////////////////////////////////////

void CpuManager::MultiCoreRunGuestThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    auto* thread = kernel.CurrentScheduler()->GetSchedulerCurrentThread();
    auto& host_context = thread->GetHostContext();
    host_context->SetRewindPoint(GuestRewindFunction, this);
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
    host_context->SetRewindPoint(GuestRewindFunction, this);
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

void CpuManager::ShutdownThread() {
    auto& kernel = system.Kernel();
    auto core = is_multicore ? kernel.CurrentPhysicalCoreIndex() : 0;
    auto* current_thread = kernel.GetCurrentEmuThread();

    Common::Fiber::YieldTo(current_thread->GetHostContext(), *core_data[core].host_context);
    UNREACHABLE();
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

    auto* current_thread = system.Kernel().CurrentScheduler()->GetIdleThread();
    Kernel::SetCurrentThread(system.Kernel(), current_thread);
    Common::Fiber::YieldTo(data.host_context, *current_thread->GetHostContext());
}

} // namespace Core
