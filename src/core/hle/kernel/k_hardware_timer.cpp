// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_hardware_timer.h"
#include "core/hle/kernel/k_scheduler.h"

namespace Kernel {

void KHardwareTimer::Initialize() {
    // Create the timing callback to register with CoreTiming.
    m_event_type = Core::Timing::CreateEvent(
        "KHardwareTimer::Callback", [](std::uintptr_t timer_handle, s64, std::chrono::nanoseconds) {
            reinterpret_cast<KHardwareTimer*>(timer_handle)->DoTask();
            return std::nullopt;
        });
}

void KHardwareTimer::Finalize() {
    m_kernel.System().CoreTiming().UnscheduleEvent(m_event_type, reinterpret_cast<uintptr_t>(this));
    m_wakeup_time = std::numeric_limits<s64>::max();
    m_event_type.reset();
}

void KHardwareTimer::DoTask() {
    // Handle the interrupt.
    {
        KScopedSchedulerLock slk{m_kernel};
        KScopedSpinLock lk(this->GetLock());

        //! Ignore this event if needed.
        if (!this->GetInterruptEnabled()) {
            return;
        }

        // Disable the timer interrupt while we handle this.
        this->DisableInterrupt();

        if (const s64 next_time = this->DoInterruptTaskImpl(GetTick());
            0 < next_time && next_time <= m_wakeup_time) {
            // We have a next time, so we should set the time to interrupt and turn the interrupt
            // on.
            this->EnableInterrupt(next_time);
        }
    }

    // Clear the timer interrupt.
    // Kernel::GetInterruptManager().ClearInterrupt(KInterruptName_NonSecurePhysicalTimer,
    //                                              GetCurrentCoreId());
}

void KHardwareTimer::EnableInterrupt(s64 wakeup_time) {
    this->DisableInterrupt();

    m_wakeup_time = wakeup_time;
    m_kernel.System().CoreTiming().ScheduleEvent(std::chrono::nanoseconds{m_wakeup_time},
                                                 m_event_type, reinterpret_cast<uintptr_t>(this),
                                                 true);
}

void KHardwareTimer::DisableInterrupt() {
    m_kernel.System().CoreTiming().UnscheduleEventWithoutWait(m_event_type,
                                                              reinterpret_cast<uintptr_t>(this));
    m_wakeup_time = std::numeric_limits<s64>::max();
}

s64 KHardwareTimer::GetTick() const {
    return m_kernel.System().CoreTiming().GetGlobalTimeNs().count();
}

bool KHardwareTimer::GetInterruptEnabled() {
    return m_wakeup_time != std::numeric_limits<s64>::max();
}

} // namespace Kernel
