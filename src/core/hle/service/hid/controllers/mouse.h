// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/settings.h"
#include "common/swap.h"
#include "core/frontend/input.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_Mouse final : public ControllerBase {
public:
    explicit Controller_Mouse(Core::System& system_);
    ~Controller_Mouse() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

private:
    struct Buttons {
        union {
            u32_le raw{};
            BitField<0, 1, u32> left;
            BitField<1, 1, u32> right;
            BitField<2, 1, u32> middle;
            BitField<3, 1, u32> forward;
            BitField<4, 1, u32> back;
        };
    };
    static_assert(sizeof(Buttons) == 0x4, "Buttons is an invalid size");

    struct Attributes {
        union {
            u32_le raw{};
            BitField<0, 1, u32> transferable;
            BitField<1, 1, u32> is_connected;
        };
    };
    static_assert(sizeof(Attributes) == 0x4, "Attributes is an invalid size");

    struct MouseState {
        s64_le sampling_number;
        s64_le sampling_number2;
        s32_le x;
        s32_le y;
        s32_le delta_x;
        s32_le delta_y;
        s32_le mouse_wheel_x;
        s32_le mouse_wheel_y;
        Buttons button;
        Attributes attribute;
    };
    static_assert(sizeof(MouseState) == 0x30, "MouseState is an invalid size");

    struct SharedMemory {
        CommonHeader header;
        std::array<MouseState, 17> mouse_states;
    };
    SharedMemory shared_memory{};

    std::unique_ptr<Input::MouseDevice> mouse_device;
    std::array<std::unique_ptr<Input::ButtonDevice>, Settings::NativeMouseButton::NumMouseButtons>
        mouse_button_devices;
};
} // namespace Service::HID
