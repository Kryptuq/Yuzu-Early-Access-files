// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hle/service/hid/controllers/console_sixaxis.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3C200;

Controller_ConsoleSixAxis::Controller_ConsoleSixAxis(Core::System& system_)
    : ControllerBase{system_} {}
Controller_ConsoleSixAxis::~Controller_ConsoleSixAxis() = default;

void Controller_ConsoleSixAxis::OnInit() {}

void Controller_ConsoleSixAxis::OnRelease() {}

void Controller_ConsoleSixAxis::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                         std::size_t size) {
    seven_six_axis.header.timestamp = core_timing.GetCPUTicks();
    seven_six_axis.header.total_entry_count = 17;

    if (!IsControllerActivated() || !is_transfer_memory_set) {
        seven_six_axis.header.entry_count = 0;
        seven_six_axis.header.last_entry_index = 0;
        return;
    }
    seven_six_axis.header.entry_count = 16;

    const auto& last_entry =
        seven_six_axis.sevensixaxis_states[seven_six_axis.header.last_entry_index];
    seven_six_axis.header.last_entry_index = (seven_six_axis.header.last_entry_index + 1) % 17;
    auto& cur_entry = seven_six_axis.sevensixaxis_states[seven_six_axis.header.last_entry_index];

    cur_entry.sampling_number = last_entry.sampling_number + 1;
    cur_entry.sampling_number2 = cur_entry.sampling_number;

    // Try to read sixaxis sensor states
    MotionDevice motion_device{};
    const auto& device = motions[0];
    if (device) {
        std::tie(motion_device.accel, motion_device.gyro, motion_device.rotation,
                 motion_device.orientation, motion_device.quaternion) = device->GetStatus();
        console_six_axis.is_seven_six_axis_sensor_at_rest = motion_device.gyro.Length2() < 0.0001f;
    }

    cur_entry.accel = motion_device.accel;
    // Zero gyro values as they just mess up with the camera
    // Note: Probably a correct sensivity setting must be set
    cur_entry.gyro = {};
    cur_entry.quaternion = {
        {
            motion_device.quaternion.xyz.y,
            motion_device.quaternion.xyz.x,
            -motion_device.quaternion.w,
        },
        -motion_device.quaternion.xyz.z,
    };

    console_six_axis.sampling_number++;
    // TODO(German77): Find the purpose of those values
    console_six_axis.verticalization_error = 0.0f;
    console_six_axis.gyro_bias = {0.0f, 0.0f, 0.0f};

    // Update console six axis shared memory
    std::memcpy(data + SHARED_MEMORY_OFFSET, &console_six_axis, sizeof(console_six_axis));
    // Update seven six axis transfer memory
    std::memcpy(transfer_memory, &seven_six_axis, sizeof(seven_six_axis));
}

void Controller_ConsoleSixAxis::OnLoadInputDevices() {
    const auto player = Settings::values.players.GetValue()[0];
    std::transform(player.motions.begin() + Settings::NativeMotion::MOTION_HID_BEGIN,
                   player.motions.begin() + Settings::NativeMotion::MOTION_HID_END, motions.begin(),
                   Input::CreateDevice<Input::MotionDevice>);
}

void Controller_ConsoleSixAxis::SetTransferMemoryPointer(u8* t_mem) {
    is_transfer_memory_set = true;
    transfer_memory = t_mem;
}

void Controller_ConsoleSixAxis::ResetTimestamp() {
    auto& cur_entry = seven_six_axis.sevensixaxis_states[seven_six_axis.header.last_entry_index];
    cur_entry.sampling_number = 0;
    cur_entry.sampling_number2 = 0;
}
} // namespace Service::HID
