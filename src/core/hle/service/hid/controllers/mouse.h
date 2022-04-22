// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
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
    explicit Controller_Mouse(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_);
    ~Controller_Mouse() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

private:
    struct MouseSharedMemory {
        // This is nn::hid::detail::MouseLifo
        Lifo<Core::HID::MouseState, hid_entry_count> mouse_lifo{};
        static_assert(sizeof(mouse_lifo) == 0x350, "mouse_lifo is an invalid size");
        INSERT_PADDING_WORDS(0x2C);
    };
    static_assert(sizeof(MouseSharedMemory) == 0x400, "MouseSharedMemory is an invalid size");

    Core::HID::MouseState next_state{};
    Core::HID::AnalogStickState last_mouse_wheel_state{};
    MouseSharedMemory* shared_memory = nullptr;
    Core::HID::EmulatedDevices* emulated_devices = nullptr;
};
} // namespace Service::HID
