// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hid/emulated_devices.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/keyboard.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3800;

Controller_Keyboard::Controller_Keyboard(Core::HID::HIDCore& hid_core_)
    : ControllerBase{hid_core_} {
    emulated_devices = hid_core.GetEmulatedDevices();
}

Controller_Keyboard::~Controller_Keyboard() = default;

void Controller_Keyboard::OnInit() {}

void Controller_Keyboard::OnRelease() {}

void Controller_Keyboard::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                   std::size_t size) {
    if (!IsControllerActivated()) {
        keyboard_lifo.buffer_count = 0;
        keyboard_lifo.buffer_tail = 0;
        std::memcpy(data + SHARED_MEMORY_OFFSET, &keyboard_lifo, sizeof(keyboard_lifo));
        return;
    }

    const auto& last_entry = keyboard_lifo.ReadCurrentEntry().state;
    next_state.sampling_number = last_entry.sampling_number + 1;

    if (Settings::values.keyboard_enabled) {
        const auto& keyboard_state = emulated_devices->GetKeyboard();
        const auto& keyboard_modifier_state = emulated_devices->GetKeyboardModifier();

        next_state.key = keyboard_state;
        next_state.modifier = keyboard_modifier_state;
        // This is always enabled on HW. Check what it actually does
        next_state.modifier.unknown.Assign(1);
    }

    keyboard_lifo.WriteNextEntry(next_state);
    std::memcpy(data + SHARED_MEMORY_OFFSET, &keyboard_lifo, sizeof(keyboard_lifo));
}

} // namespace Service::HID
