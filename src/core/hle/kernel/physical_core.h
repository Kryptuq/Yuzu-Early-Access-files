// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include "core/arm/arm_interface.h"

namespace Common {
class SpinLock;
}

namespace Kernel {
class KScheduler;
} // namespace Kernel

namespace Core {
class CPUInterruptHandler;
class ExclusiveMonitor;
class System;
} // namespace Core

namespace Kernel {

class PhysicalCore {
public:
    PhysicalCore(std::size_t core_index_, Core::System& system_, KScheduler& scheduler_,
                 Core::CPUInterrupts& interrupts_);
    ~PhysicalCore();

    PhysicalCore(const PhysicalCore&) = delete;
    PhysicalCore& operator=(const PhysicalCore&) = delete;

    PhysicalCore(PhysicalCore&&) = default;
    PhysicalCore& operator=(PhysicalCore&&) = delete;

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
    Core::CPUInterrupts& interrupts;
    std::unique_ptr<Common::SpinLock> guard;
    std::unique_ptr<Core::ARM_Interface> arm_interface;
};

} // namespace Kernel
