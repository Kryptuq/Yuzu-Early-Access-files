// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Core::HID {
class EmulatedDevices;
struct KeyboardModifier;
struct KeyboardKey;
} // namespace Core::HID

namespace Service::HID {
class Controller_Keyboard final : public ControllerBase {
public:
    explicit Controller_Keyboard(Core::HID::HIDCore& hid_core_);
    ~Controller_Keyboard() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

private:
    // This is nn::hid::detail::KeyboardState
    struct KeyboardState {
        s64 sampling_number;
        Core::HID::KeyboardModifier modifier;
        Core::HID::KeyboardAttribute attribute;
        Core::HID::KeyboardKey key;
    };
    static_assert(sizeof(KeyboardState) == 0x30, "KeyboardState is an invalid size");

    // This is nn::hid::detail::KeyboardLifo
    Lifo<KeyboardState, hid_entry_count> keyboard_lifo{};
    static_assert(sizeof(keyboard_lifo) == 0x3D8, "keyboard_lifo is an invalid size");
    KeyboardState next_state{};

    Core::HID::EmulatedDevices* emulated_devices;
};
} // namespace Service::HID
