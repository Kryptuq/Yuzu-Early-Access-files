// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hid/emulated_console.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/console_sixaxis.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3C200;

Controller_ConsoleSixAxis::Controller_ConsoleSixAxis(Core::HID::HIDCore& hid_core_)
    : ControllerBase{hid_core_} {
    console = hid_core.GetEmulatedConsole();
}

Controller_ConsoleSixAxis::~Controller_ConsoleSixAxis() = default;

void Controller_ConsoleSixAxis::OnInit() {}

void Controller_ConsoleSixAxis::OnRelease() {}

void Controller_ConsoleSixAxis::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                         std::size_t size) {
    if (!IsControllerActivated() || !is_transfer_memory_set) {
        seven_sixaxis_lifo.buffer_count = 0;
        seven_sixaxis_lifo.buffer_tail = 0;
        return;
    }

    const auto& last_entry = seven_sixaxis_lifo.ReadCurrentEntry().state;
    next_seven_sixaxis_state.sampling_number = last_entry.sampling_number + 1;

    // Try to read sixaxis sensor states
    const auto motion_status = console->GetMotion();

    console_six_axis.is_seven_six_axis_sensor_at_rest = motion_status.is_at_rest;

    next_seven_sixaxis_state.accel = motion_status.accel;
    // Zero gyro values as they just mess up with the camera
    // Note: Probably a correct sensivity setting must be set
    next_seven_sixaxis_state.gyro = {};
    next_seven_sixaxis_state.quaternion = {
        {
            motion_status.quaternion.xyz.y,
            motion_status.quaternion.xyz.x,
            -motion_status.quaternion.w,
        },
        -motion_status.quaternion.xyz.z,
    };

    console_six_axis.sampling_number++;
    // TODO(German77): Find the purpose of those values
    console_six_axis.verticalization_error = 0.0f;
    console_six_axis.gyro_bias = {0.0f, 0.0f, 0.0f};

    // Update console six axis shared memory
    std::memcpy(data + SHARED_MEMORY_OFFSET, &console_six_axis, sizeof(console_six_axis));
    // Update seven six axis transfer memory
    seven_sixaxis_lifo.WriteNextEntry(next_seven_sixaxis_state);
    std::memcpy(transfer_memory, &seven_sixaxis_lifo, sizeof(seven_sixaxis_lifo));
}

void Controller_ConsoleSixAxis::SetTransferMemoryPointer(u8* t_mem) {
    is_transfer_memory_set = true;
    transfer_memory = t_mem;
}

void Controller_ConsoleSixAxis::ResetTimestamp() {
    seven_sixaxis_lifo.buffer_count = 0;
    seven_sixaxis_lifo.buffer_tail = 0;
}
} // namespace Service::HID
