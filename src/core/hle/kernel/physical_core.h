// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>

#include "core/arm/arm_interface.h"

namespace Kernel {
class KScheduler;
} // namespace Kernel

namespace Core {
class ExclusiveMonitor;
class System;
} // namespace Core

namespace Kernel {

class PhysicalCore {
public:
    PhysicalCore(std::size_t core_index_, Core::System& system_, KScheduler& scheduler_);
    ~PhysicalCore();

    YUZU_NON_COPYABLE(PhysicalCore);
    YUZU_NON_MOVEABLE(PhysicalCore);

    /// Initialize the core for the specified parameters.
    void Initialize(bool is_64_bit);

    /// Execute current jit state
    void Run();

    void Idle();

    /// Interrupt this physical core.
    void Interrupt();

    /// Clear this core's interrupt
    void ClearInterrupt();

    /// Check if this core is interrupted
    bool IsInterrupted() const;

    bool IsInitialized() const {
        return m_arm_interface != nullptr;
    }

    Core::ARM_Interface& ArmInterface() {
        return *m_arm_interface;
    }

    const Core::ARM_Interface& ArmInterface() const {
        return *m_arm_interface;
    }

    std::size_t CoreIndex() const {
        return m_core_index;
    }

    Kernel::KScheduler& Scheduler() {
        return m_scheduler;
    }

    const Kernel::KScheduler& Scheduler() const {
        return m_scheduler;
    }

private:
    const std::size_t m_core_index;
    Core::System& m_system;
    Kernel::KScheduler& m_scheduler;

    std::mutex m_guard;
    std::condition_variable m_on_interrupt;
    std::unique_ptr<Core::ARM_Interface> m_arm_interface;
    bool m_is_interrupted{};
};

} // namespace Kernel
