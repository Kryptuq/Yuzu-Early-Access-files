// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/point.h"
#include "common/swap.h"
#include "core/hid/hid_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Core::HID {
class EmulatedConsole;
} // namespace Core::HID

namespace Service::HID {
class Controller_Touchscreen final : public ControllerBase {
public:
    // This is nn::hid::TouchScreenModeForNx
    enum class TouchScreenModeForNx : u8 {
        UseSystemSetting,
        Finger,
        Heat2,
    };

    // This is nn::hid::TouchScreenConfigurationForNx
    struct TouchScreenConfigurationForNx {
        TouchScreenModeForNx mode;
        INSERT_PADDING_BYTES_NOINIT(0x7);
        INSERT_PADDING_BYTES_NOINIT(0xF); // Reserved
    };
    static_assert(sizeof(TouchScreenConfigurationForNx) == 0x17,
                  "TouchScreenConfigurationForNx is an invalid size");

    explicit Controller_Touchscreen(Core::HID::HIDCore& hid_core_);
    ~Controller_Touchscreen() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

private:
    static constexpr std::size_t MAX_FINGERS = 16;

    // This is nn::hid::TouchScreenState
    struct TouchScreenState {
        s64 sampling_number;
        s32 entry_count;
        INSERT_PADDING_BYTES(4); // Reserved
        std::array<Core::HID::TouchState, MAX_FINGERS> states;
    };
    static_assert(sizeof(TouchScreenState) == 0x290, "TouchScreenState is an invalid size");

    // This is nn::hid::detail::TouchScreenLifo
    Lifo<TouchScreenState, hid_entry_count> touch_screen_lifo{};
    static_assert(sizeof(touch_screen_lifo) == 0x2C38, "touch_screen_lifo is an invalid size");
    TouchScreenState next_state{};

    std::array<Core::HID::TouchFinger, MAX_FINGERS> fingers;
    Core::HID::EmulatedConsole* console;
};
} // namespace Service::HID
