// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_64.h"
#include "core/core.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

PhysicalCore::PhysicalCore(std::size_t core_index, Core::System& system, KScheduler& scheduler)
    : m_core_index{core_index}, m_system{system}, m_scheduler{scheduler} {
#if defined(ARCHITECTURE_x86_64) || defined(ARCHITECTURE_arm64)
    // TODO(bunnei): Initialization relies on a core being available. We may later replace this with
    // a 32-bit instance of Dynarmic. This should be abstracted out to a CPU manager.
    auto& kernel = system.Kernel();
    m_arm_interface = std::make_unique<Core::ARM_Dynarmic_64>(
        system, kernel.IsMulticore(), kernel.GetExclusiveMonitor(), m_core_index);
#else
#error Platform not supported yet.
#endif
}

PhysicalCore::~PhysicalCore() = default;

void PhysicalCore::Initialize(bool is_64_bit) {
#if defined(ARCHITECTURE_x86_64) || defined(ARCHITECTURE_arm64)
    auto& kernel = m_system.Kernel();
    if (!is_64_bit) {
        // We already initialized a 64-bit core, replace with a 32-bit one.
        m_arm_interface = std::make_unique<Core::ARM_Dynarmic_32>(
            m_system, kernel.IsMulticore(), kernel.GetExclusiveMonitor(), m_core_index);
    }
#else
#error Platform not supported yet.
#endif
}

void PhysicalCore::Run() {
    m_arm_interface->Run();
    m_arm_interface->ClearExclusiveState();
}

void PhysicalCore::Idle() {
    std::unique_lock lk{m_guard};
    m_on_interrupt.wait(lk, [this] { return m_is_interrupted; });
}

bool PhysicalCore::IsInterrupted() const {
    return m_is_interrupted;
}

void PhysicalCore::Interrupt() {
    std::unique_lock lk{m_guard};
    m_is_interrupted = true;
    m_arm_interface->SignalInterrupt();
    m_on_interrupt.notify_all();
}

void PhysicalCore::ClearInterrupt() {
    std::unique_lock lk{m_guard};
    m_is_interrupted = false;
    m_arm_interface->ClearInterrupt();
}

} // namespace Kernel
