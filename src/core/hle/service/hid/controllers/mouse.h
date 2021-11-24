// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Core::HID {
class EmulatedDevices;
struct MouseState;
struct AnalogStickState;
} // namespace Core::HID

namespace Service::HID {
class Controller_Mouse final : public ControllerBase {
public:
    explicit Controller_Mouse(Core::HID::HIDCore& hid_core_);
    ~Controller_Mouse() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

private:
    // This is nn::hid::detail::MouseLifo
    Lifo<Core::HID::MouseState, hid_entry_count> mouse_lifo{};
    static_assert(sizeof(mouse_lifo) == 0x350, "mouse_lifo is an invalid size");
    Core::HID::MouseState next_state{};

    Core::HID::AnalogStickState last_mouse_wheel_state;
    Core::HID::EmulatedDevices* emulated_devices;
};
} // namespace Service::HID
