// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hid/emulated_console.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/console_sixaxis.h"
#include "core/memory.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3C200;

Controller_ConsoleSixAxis::Controller_ConsoleSixAxis(Core::System& system_, u8* raw_shared_memory_)
    : ControllerBase{system_.HIDCore()}, system{system_} {
    console = hid_core.GetEmulatedConsole();
    static_assert(SHARED_MEMORY_OFFSET + sizeof(ConsoleSharedMemory) < shared_memory_size,
                  "ConsoleSharedMemory is bigger than the shared memory");
    shared_memory = std::construct_at(
        reinterpret_cast<ConsoleSharedMemory*>(raw_shared_memory_ + SHARED_MEMORY_OFFSET));
}

Controller_ConsoleSixAxis::~Controller_ConsoleSixAxis() = default;

void Controller_ConsoleSixAxis::OnInit() {}

void Controller_ConsoleSixAxis::OnRelease() {}

void Controller_ConsoleSixAxis::OnUpdate(const Core::Timing::CoreTiming& core_timing) {
    if (!IsControllerActivated() || transfer_memory == 0) {
        seven_sixaxis_lifo.buffer_count = 0;
        seven_sixaxis_lifo.buffer_tail = 0;
        return;
    }

    const auto& last_entry = seven_sixaxis_lifo.ReadCurrentEntry().state;
    next_seven_sixaxis_state.sampling_number = last_entry.sampling_number + 1;

    const auto motion_status = console->GetMotion();
    last_global_timestamp = core_timing.GetGlobalTimeNs().count();

    // This value increments every time the switch goes to sleep
    next_seven_sixaxis_state.unknown = 1;
    next_seven_sixaxis_state.timestamp = last_global_timestamp - last_saved_timestamp;
    next_seven_sixaxis_state.accel = motion_status.accel;
    next_seven_sixaxis_state.gyro = motion_status.gyro;
    next_seven_sixaxis_state.quaternion = {
        {
            motion_status.quaternion.xyz.y,
            motion_status.quaternion.xyz.x,
            -motion_status.quaternion.w,
        },
        -motion_status.quaternion.xyz.z,
    };

    shared_memory->sampling_number++;
    shared_memory->is_seven_six_axis_sensor_at_rest = motion_status.is_at_rest;
    shared_memory->verticalization_error = motion_status.verticalization_error;
    shared_memory->gyro_bias = motion_status.gyro_bias;

    // Update seven six axis transfer memory
    seven_sixaxis_lifo.WriteNextEntry(next_seven_sixaxis_state);
    system.Memory().WriteBlock(transfer_memory, &seven_sixaxis_lifo, sizeof(seven_sixaxis_lifo));
}

void Controller_ConsoleSixAxis::SetTransferMemoryAddress(VAddr t_mem) {
    transfer_memory = t_mem;
}

void Controller_ConsoleSixAxis::ResetTimestamp() {
    last_saved_timestamp = last_global_timestamp;
}
} // namespace Service::HID
