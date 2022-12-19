// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_hardware_timer_base.h"

namespace Core::Timing {
struct EventType;
} // namespace Core::Timing

namespace Kernel {

class KHardwareTimer : /* public KInterruptTask, */ public KHardwareTimerBase {
public:
    explicit KHardwareTimer(KernelCore& kernel) : KHardwareTimerBase{kernel} {}

    // Public API.
    void Initialize();
    void Finalize();

    s64 GetCount() const {
        return GetTick();
    }

    void RegisterTask(KTimerTask* task, s64 time_from_now) {
        this->RegisterAbsoluteTask(task, GetTick() + time_from_now);
    }

    void RegisterAbsoluteTask(KTimerTask* task, s64 task_time) {
        KScopedDisableDispatch dd{m_kernel};
        KScopedSpinLock lk{this->GetLock()};

        if (this->RegisterAbsoluteTaskImpl(task, task_time)) {
            if (task_time <= m_wakeup_time) {
                this->EnableInterrupt(task_time);
            }
        }
    }

private:
    void EnableInterrupt(s64 wakeup_time);
    void DisableInterrupt();
    bool GetInterruptEnabled();
    s64 GetTick() const;
    void DoTask();

private:
    // Absolute time in nanoseconds
    s64 m_wakeup_time{std::numeric_limits<s64>::max()};
    std::shared_ptr<Core::Timing::EventType> m_event_type{};
};

} // namespace Kernel
