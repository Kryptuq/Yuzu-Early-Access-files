// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/point.h"
#include "common/swap.h"
#include "core/frontend/input.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_Touchscreen final : public ControllerBase {
public:
    enum class TouchScreenModeForNx : u8 {
        UseSystemSetting,
        Finger,
        Heat2,
    };

    struct TouchScreenConfigurationForNx {
        TouchScreenModeForNx mode;
        INSERT_PADDING_BYTES_NOINIT(0x7);
        INSERT_PADDING_BYTES_NOINIT(0xF); // Reserved
    };
    static_assert(sizeof(TouchScreenConfigurationForNx) == 0x17,
                  "TouchScreenConfigurationForNx is an invalid size");

    explicit Controller_Touchscreen(Core::System& system_);
    ~Controller_Touchscreen() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

private:
    static constexpr std::size_t MAX_FINGERS = 16;

    // Returns an unused finger id, if there is no fingers available std::nullopt will be returned
    std::optional<std::size_t> GetUnusedFingerID() const;

    // If the touch is new it tries to assing a new finger id, if there is no fingers avaliable no
    // changes will be made. Updates the coordinates if the finger id it's already set. If the touch
    // ends delays the output by one frame to set the end_touch flag before finally freeing the
    // finger id
    std::size_t UpdateTouchInputEvent(const std::tuple<float, float, bool>& touch_input,
                                      std::size_t finger_id);

    struct Attributes {
        union {
            u32 raw{};
            BitField<0, 1, u32> start_touch;
            BitField<1, 1, u32> end_touch;
        };
    };
    static_assert(sizeof(Attributes) == 0x4, "Attributes is an invalid size");

    struct TouchState {
        u64_le delta_time;
        Attributes attribute;
        u32_le finger;
        Common::Point<u32_le> position;
        u32_le diameter_x;
        u32_le diameter_y;
        u32_le rotation_angle;
    };
    static_assert(sizeof(TouchState) == 0x28, "Touchstate is an invalid size");

    struct TouchScreenEntry {
        s64_le sampling_number;
        s64_le sampling_number2;
        s32_le entry_count;
        std::array<TouchState, MAX_FINGERS> states;
    };
    static_assert(sizeof(TouchScreenEntry) == 0x298, "TouchScreenEntry is an invalid size");

    struct TouchScreenSharedMemory {
        CommonHeader header;
        std::array<TouchScreenEntry, 17> shared_memory_entries{};
        INSERT_PADDING_BYTES(0x3c8);
    };
    static_assert(sizeof(TouchScreenSharedMemory) == 0x3000,
                  "TouchScreenSharedMemory is an invalid size");

    struct Finger {
        u64_le last_touch{};
        Common::Point<float> position;
        u32_le id{};
        bool pressed{};
        Attributes attribute;
    };

    TouchScreenSharedMemory shared_memory{};
    std::unique_ptr<Input::TouchDevice> touch_mouse_device;
    std::unique_ptr<Input::TouchDevice> touch_udp_device;
    std::unique_ptr<Input::TouchDevice> touch_btn_device;
    std::array<std::size_t, MAX_FINGERS> mouse_finger_id;
    std::array<std::size_t, MAX_FINGERS> keyboard_finger_id;
    std::array<std::size_t, MAX_FINGERS> udp_finger_id;
    std::array<Finger, MAX_FINGERS> fingers;
};
} // namespace Service::HID
