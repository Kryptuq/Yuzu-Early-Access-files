// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_64.h"
#include "core/core.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

PhysicalCore::PhysicalCore(std::size_t core_index_, Core::System& system_, KScheduler& scheduler_)
    : core_index{core_index_}, system{system_}, scheduler{scheduler_} {
#ifdef ARCHITECTURE_x86_64
    // TODO(bunnei): Initialization relies on a core being available. We may later replace this with
    // a 32-bit instance of Dynarmic. This should be abstracted out to a CPU manager.
    auto& kernel = system.Kernel();
    arm_interface = std::make_unique<Core::ARM_Dynarmic_64>(
        system, kernel.IsMulticore(), kernel.GetExclusiveMonitor(), core_index);
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
            system, kernel.IsMulticore(), kernel.GetExclusiveMonitor(), core_index);
    }
#else
#error Platform not supported yet.
#endif
}

void PhysicalCore::Run() {
    arm_interface->Run();
    arm_interface->ClearExclusiveState();
}

void PhysicalCore::Idle() {
    std::unique_lock lk{guard};
    on_interrupt.wait(lk, [this] { return is_interrupted; });
}

bool PhysicalCore::IsInterrupted() const {
    return is_interrupted;
}

void PhysicalCore::Interrupt() {
    std::unique_lock lk{guard};
    is_interrupted = true;
    arm_interface->SignalInterrupt();
    on_interrupt.notify_all();
}

void PhysicalCore::ClearInterrupt() {
    std::unique_lock lk{guard};
    is_interrupted = false;
    arm_interface->ClearInterrupt();
    on_interrupt.notify_all();
}

} // namespace Kernel
