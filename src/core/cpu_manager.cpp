// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/fiber.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "common/thread.h"
#include "core/arm/exclusive_monitor.h"
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

void CpuManager::ThreadStart(CpuManager& cpu_manager, std::size_t core) {
    cpu_manager.RunThread(core);
}

void CpuManager::Initialize() {
    running_mode = true;
    if (is_multicore) {
        for (std::size_t core = 0; core < Core::Hardware::NUM_CPU_CORES; core++) {
            core_data[core].host_thread =
                std::make_unique<std::thread>(ThreadStart, std::ref(*this), core);
        }
    } else {
        core_data[0].host_thread = std::make_unique<std::thread>(ThreadStart, std::ref(*this), 0);
    }
}

void CpuManager::Shutdown() {
    running_mode = false;
    Pause(false);
    if (is_multicore) {
        for (auto& data : core_data) {
            data.host_thread->join();
            data.host_thread.reset();
        }
    } else {
        core_data[0].host_thread->join();
        core_data[0].host_thread.reset();
    }
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
        physical_core->ArmInterface().ClearExclusiveState();
        kernel.CurrentScheduler()->RescheduleCurrentCore();
    }
}

void CpuManager::MultiCoreRunIdleThread() {
    auto& kernel = system.Kernel();
    while (true) {
        auto& physical_core = kernel.CurrentPhysicalCore();
        physical_core.Idle();
        kernel.CurrentScheduler()->RescheduleCurrentCore();
    }
}

void CpuManager::MultiCoreRunSuspendThread() {
    auto& kernel = system.Kernel();
    kernel.CurrentScheduler()->OnThreadStart();
    while (true) {
        auto core = kernel.GetCurrentHostThreadID();
        auto& scheduler = *kernel.CurrentScheduler();
        Kernel::KThread* current_thread = scheduler.GetCurrentThread();
        Common::Fiber::YieldTo(current_thread->GetHostContext(), *core_data[core].host_context);
        ASSERT(scheduler.ContextSwitchPending());
        ASSERT(core == kernel.GetCurrentHostThreadID());
        scheduler.RescheduleCurrentCore();
    }
}

void CpuManager::MultiCorePause(bool paused) {
    if (!paused) {
        bool all_not_barrier = false;
        while (!all_not_barrier) {
            all_not_barrier = true;
            for (const auto& data : core_data) {
                all_not_barrier &= !data.is_running.load() && data.initialized.load();
            }
        }
        for (auto& data : core_data) {
            data.enter_barrier->Set();
        }
        if (paused_state.load()) {
            bool all_barrier = false;
            while (!all_barrier) {
                all_barrier = true;
                for (const auto& data : core_data) {
                    all_barrier &= data.is_paused.load() && data.initialized.load();
                }
            }
            for (auto& data : core_data) {
                data.exit_barrier->Set();
            }
        }
    } else {
        /// Wait until all cores are paused.
        bool all_barrier = false;
        while (!all_barrier) {
            all_barrier = true;
            for (const auto& data : core_data) {
                all_barrier &= data.is_paused.load() && data.initialized.load();
            }
        }
        /// Don't release the barrier
    }
    paused_state = paused;
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

void CpuManager::SingleCorePause(bool paused) {
    if (!paused) {
        bool all_not_barrier = false;
        while (!all_not_barrier) {
            all_not_barrier = !core_data[0].is_running.load() && core_data[0].initialized.load();
        }
        core_data[0].enter_barrier->Set();
        if (paused_state.load()) {
            bool all_barrier = false;
            while (!all_barrier) {
                all_barrier = core_data[0].is_paused.load() && core_data[0].initialized.load();
            }
            core_data[0].exit_barrier->Set();
        }
    } else {
        /// Wait until all cores are paused.
        bool all_barrier = false;
        while (!all_barrier) {
            all_barrier = core_data[0].is_paused.load() && core_data[0].initialized.load();
        }
        /// Don't release the barrier
    }
    paused_state = paused;
}

void CpuManager::Pause(bool paused) {
    if (is_multicore) {
        MultiCorePause(paused);
    } else {
        SingleCorePause(paused);
    }
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
    data.enter_barrier = std::make_unique<Common::Event>();
    data.exit_barrier = std::make_unique<Common::Event>();
    data.host_context = Common::Fiber::ThreadToFiber();
    data.is_running = false;
    data.initialized = true;
    const bool sc_sync = !is_async_gpu && !is_multicore;
    bool sc_sync_first_use = sc_sync;

    // Cleanup
    SCOPE_EXIT({
        data.host_context->Exit();
        data.enter_barrier.reset();
        data.exit_barrier.reset();
        data.initialized = false;
        MicroProfileOnThreadExit();
    });

    /// Running
    while (running_mode) {
        data.is_running = false;
        data.enter_barrier->Wait();
        if (sc_sync_first_use) {
            system.GPU().ObtainContext();
            sc_sync_first_use = false;
        }

        // Abort if emulation was killed before the session really starts
        if (!system.IsPoweredOn()) {
            return;
        }

        auto current_thread = system.Kernel().CurrentScheduler()->GetCurrentThread();
        data.is_running = true;
        Common::Fiber::YieldTo(data.host_context, *current_thread->GetHostContext());
        data.is_running = false;
        data.is_paused = true;
        data.exit_barrier->Wait();
        data.is_paused = false;
    }
}

} // namespace Core
