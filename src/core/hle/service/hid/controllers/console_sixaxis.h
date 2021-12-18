// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/common_types.h"
#include "common/quaternion.h"
#include "core/hid/hid_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Core::HID {
class EmulatedConsole;
} // namespace Core::HID

namespace Service::HID {
class Controller_ConsoleSixAxis final : public ControllerBase {
public:
    explicit Controller_ConsoleSixAxis(Core::HID::HIDCore& hid_core_);
    ~Controller_ConsoleSixAxis() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, size_t size) override;

    // Called on InitializeSevenSixAxisSensor
    void SetTransferMemoryPointer(u8* t_mem);

    // Called on ResetSevenSixAxisSensorTimestamp
    void ResetTimestamp();

private:
    struct SevenSixAxisState {
        INSERT_PADDING_WORDS(2); // unused
        u64 timestamp{};
        u64 sampling_number{};
        u64 unknown{};
        Common::Vec3f accel{};
        Common::Vec3f gyro{};
        Common::Quaternion<f32> quaternion{};
    };
    static_assert(sizeof(SevenSixAxisState) == 0x48, "SevenSixAxisState is an invalid size");

    // This is nn::hid::detail::ConsoleSixAxisSensorSharedMemoryFormat
    struct ConsoleSharedMemory {
        u64 sampling_number{};
        bool is_seven_six_axis_sensor_at_rest{};
        INSERT_PADDING_BYTES(3); // padding
        f32 verticalization_error{};
        Common::Vec3f gyro_bias{};
        INSERT_PADDING_BYTES(4); // padding
    };
    static_assert(sizeof(ConsoleSharedMemory) == 0x20, "ConsoleSharedMemory is an invalid size");

    Lifo<SevenSixAxisState, 0x21> seven_sixaxis_lifo{};
    static_assert(sizeof(seven_sixaxis_lifo) == 0xA70, "SevenSixAxisState is an invalid size");

    Core::HID::EmulatedConsole* console;
    u8* transfer_memory = nullptr;
    bool is_transfer_memory_set = false;
    u64 last_saved_timestamp{};
    u64 last_global_timestamp{};
    ConsoleSharedMemory console_six_axis{};
    SevenSixAxisState next_seven_sixaxis_state{};
};
} // namespace Service::HID
