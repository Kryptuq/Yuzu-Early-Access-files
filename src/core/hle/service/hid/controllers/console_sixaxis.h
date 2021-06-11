// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/quaternion.h"
#include "core/frontend/input.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_ConsoleSixAxis final : public ControllerBase {
public:
    explicit Controller_ConsoleSixAxis(Core::System& system_);
    ~Controller_ConsoleSixAxis() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

    // Called on InitializeSevenSixAxisSensor
    void SetTransferMemoryPointer(u8* t_mem);

    // Called on ResetSevenSixAxisSensorTimestamp
    void ResetTimestamp();

private:
    struct SevenSixAxisState {
        INSERT_PADDING_WORDS(4); // unused
        s64_le sampling_number{};
        s64_le sampling_number2{};
        u64 unknown{};
        Common::Vec3f accel{};
        Common::Vec3f gyro{};
        Common::Quaternion<f32> quaternion{};
    };
    static_assert(sizeof(SevenSixAxisState) == 0x50, "SevenSixAxisState is an invalid size");

    struct SevenSixAxisMemory {
        CommonHeader header{};
        std::array<SevenSixAxisState, 0x21> sevensixaxis_states{};
    };
    static_assert(sizeof(SevenSixAxisMemory) == 0xA70, "SevenSixAxisMemory is an invalid size");

    struct ConsoleSharedMemory {
        u64_le sampling_number{};
        bool is_seven_six_axis_sensor_at_rest{};
        f32 verticalization_error{};
        Common::Vec3f gyro_bias{};
    };
    static_assert(sizeof(ConsoleSharedMemory) == 0x20, "ConsoleSharedMemory is an invalid size");

    struct MotionDevice {
        Common::Vec3f accel;
        Common::Vec3f gyro;
        Common::Vec3f rotation;
        std::array<Common::Vec3f, 3> orientation;
        Common::Quaternion<f32> quaternion;
    };

    using MotionArray =
        std::array<std::unique_ptr<Input::MotionDevice>, Settings::NativeMotion::NUM_MOTIONS_HID>;
    MotionArray motions;
    u8* transfer_memory = nullptr;
    bool is_transfer_memory_set = false;
    ConsoleSharedMemory console_six_axis{};
    SevenSixAxisMemory seven_six_axis{};
};
} // namespace Service::HID
