// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hardware_interrupt_manager.h"
#include "core/hle/service/nvdrv/nvdrv_interface.h"
#include "core/hle/service/sm/sm.h"

namespace Core::Hardware {

InterruptManager::InterruptManager(Core::System& system_in) : system(system_in) {
    gpu_interrupt_event = Core::Timing::CreateEvent(
        "GPUInterrupt", [this](std::uintptr_t message, std::chrono::nanoseconds) {
            auto nvdrv = system.ServiceManager().GetService<Service::Nvidia::NVDRV>("nvdrv");
            const u32 syncpt = static_cast<u32>(message >> 32);
            const u32 value = static_cast<u32>(message);
            nvdrv->SignalGPUInterruptSyncpt(syncpt, value);
        });
}

InterruptManager::~InterruptManager() = default;

void InterruptManager::GPUInterruptSyncpt(const u32 syncpoint_id, const u32 value) {
    const u64 msg = (static_cast<u64>(syncpoint_id) << 32ULL) | value;
    system.CoreTiming().ScheduleEvent(std::chrono::nanoseconds{10}, gpu_interrupt_event, msg);
}

} // namespace Core::Hardware
