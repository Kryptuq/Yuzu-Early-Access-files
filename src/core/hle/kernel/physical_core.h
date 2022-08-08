// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
        return arm_interface != nullptr;
    }

    Core::ARM_Interface& ArmInterface() {
        return *arm_interface;
    }

    const Core::ARM_Interface& ArmInterface() const {
        return *arm_interface;
    }

    bool IsMainCore() const {
        return core_index == 0;
    }

    bool IsSystemCore() const {
        return core_index == 3;
    }

    std::size_t CoreIndex() const {
        return core_index;
    }

    Kernel::KScheduler& Scheduler() {
        return scheduler;
    }

    const Kernel::KScheduler& Scheduler() const {
        return scheduler;
    }

private:
    const std::size_t core_index;
    Core::System& system;
    Kernel::KScheduler& scheduler;

    std::mutex guard;
    std::condition_variable on_interrupt;
    std::unique_ptr<Core::ARM_Interface> arm_interface;
    bool is_interrupted;
};

} // namespace Kernel
