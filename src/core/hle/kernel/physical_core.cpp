// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/arm/cpu_interrupt_handler.h"
#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_64.h"
#include "core/core.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

PhysicalCore::PhysicalCore(std::size_t core_index_, Core::System& system_, KScheduler& scheduler_,
                           Core::CPUInterrupts& interrupts_)
    : core_index{core_index_}, system{system_}, scheduler{scheduler_},
      interrupts{interrupts_}, guard{std::make_unique<std::mutex>()} {
#ifdef ARCHITECTURE_x86_64
    // TODO(bunnei): Initialization relies on a core being available. We may later replace this with
    // a 32-bit instance of Dynarmic. This should be abstracted out to a CPU manager.
    auto& kernel = system.Kernel();
    arm_interface = std::make_unique<Core::ARM_Dynarmic_64>(
        system, interrupts, kernel.IsMulticore(), kernel.GetExclusiveMonitor(), core_index);
#else
#error Platform not supported yet.
#endif
}

PhysicalCore::~PhysicalCore() = default;

void PhysicalCore::Initialize([[maybe_unused]] bool is_64_bit) {
#ifdef ARCHITECTURE_x86_64
    auto& kernel = system.Kernel();
    if (!is_64_bit) {
        // We already initialized a 64-bit core, replace with a 32-bit one.
        arm_interface = std::make_unique<Core::ARM_Dynarmic_32>(
            system, interrupts, kernel.IsMulticore(), kernel.GetExclusiveMonitor(), core_index);
    }
#else
#error Platform not supported yet.
#endif
}

void PhysicalCore::Run() {
    arm_interface->Run();
}

void PhysicalCore::Idle() {
    interrupts[core_index].AwaitInterrupt();
}

bool PhysicalCore::IsInterrupted() const {
    return interrupts[core_index].IsInterrupted();
}

void PhysicalCore::Interrupt() {
    guard->lock();
    interrupts[core_index].SetInterrupt(true);
    arm_interface->SignalInterrupt();
    guard->unlock();
}

void PhysicalCore::ClearInterrupt() {
    guard->lock();
    interrupts[core_index].SetInterrupt(false);
    guard->unlock();
}

} // namespace Kernel
