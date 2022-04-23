// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
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
    explicit Controller_Keyboard(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_);
    ~Controller_Keyboard() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

private:
    // This is nn::hid::detail::KeyboardState
    struct KeyboardState {
        s64 sampling_number{};
        Core::HID::KeyboardModifier modifier{};
        Core::HID::KeyboardAttribute attribute{};
        Core::HID::KeyboardKey key{};
    };
    static_assert(sizeof(KeyboardState) == 0x30, "KeyboardState is an invalid size");

    struct KeyboardSharedMemory {
        // This is nn::hid::detail::KeyboardLifo
        Lifo<KeyboardState, hid_entry_count> keyboard_lifo{};
        static_assert(sizeof(keyboard_lifo) == 0x3D8, "keyboard_lifo is an invalid size");
        INSERT_PADDING_WORDS(0xA);
    };
    static_assert(sizeof(KeyboardSharedMemory) == 0x400, "KeyboardSharedMemory is an invalid size");

    KeyboardState next_state{};
    KeyboardSharedMemory* shared_memory = nullptr;
    Core::HID::EmulatedDevices* emulated_devices = nullptr;
};
} // namespace Service::HID
