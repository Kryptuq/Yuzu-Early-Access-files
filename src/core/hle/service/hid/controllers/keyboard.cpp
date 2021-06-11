// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hle/service/hid/controllers/keyboard.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3800;
constexpr u8 KEYS_PER_BYTE = 8;

Controller_Keyboard::Controller_Keyboard(Core::System& system_) : ControllerBase{system_} {}
Controller_Keyboard::~Controller_Keyboard() = default;

void Controller_Keyboard::OnInit() {}

void Controller_Keyboard::OnRelease() {}

void Controller_Keyboard::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                   std::size_t size) {
    shared_memory.header.timestamp = core_timing.GetCPUTicks();
    shared_memory.header.total_entry_count = 17;

    if (!IsControllerActivated()) {
        shared_memory.header.entry_count = 0;
        shared_memory.header.last_entry_index = 0;
        return;
    }
    shared_memory.header.entry_count = 16;

    const auto& last_entry = shared_memory.pad_states[shared_memory.header.last_entry_index];
    shared_memory.header.last_entry_index = (shared_memory.header.last_entry_index + 1) % 17;
    auto& cur_entry = shared_memory.pad_states[shared_memory.header.last_entry_index];

    cur_entry.sampling_number = last_entry.sampling_number + 1;
    cur_entry.sampling_number2 = cur_entry.sampling_number;

    cur_entry.key.fill(0);
    if (Settings::values.keyboard_enabled) {
        for (std::size_t i = 0; i < keyboard_keys.size(); ++i) {
            auto& entry = cur_entry.key[i / KEYS_PER_BYTE];
            entry = static_cast<u8>(entry | (keyboard_keys[i]->GetStatus() << (i % KEYS_PER_BYTE)));
        }

        using namespace Settings::NativeKeyboard;

        // TODO: Assign the correct key to all modifiers
        cur_entry.modifier.control.Assign(keyboard_mods[LeftControl]->GetStatus());
        cur_entry.modifier.shift.Assign(keyboard_mods[LeftShift]->GetStatus());
        cur_entry.modifier.left_alt.Assign(keyboard_mods[LeftAlt]->GetStatus());
        cur_entry.modifier.right_alt.Assign(keyboard_mods[RightAlt]->GetStatus());
        cur_entry.modifier.gui.Assign(0);
        cur_entry.modifier.caps_lock.Assign(keyboard_mods[CapsLock]->GetStatus());
        cur_entry.modifier.scroll_lock.Assign(keyboard_mods[ScrollLock]->GetStatus());
        cur_entry.modifier.num_lock.Assign(keyboard_mods[NumLock]->GetStatus());
        cur_entry.modifier.katakana.Assign(0);
        cur_entry.modifier.hiragana.Assign(0);
    }
    std::memcpy(data + SHARED_MEMORY_OFFSET, &shared_memory, sizeof(SharedMemory));
}

void Controller_Keyboard::OnLoadInputDevices() {
    std::transform(Settings::values.keyboard_keys.begin(), Settings::values.keyboard_keys.end(),
                   keyboard_keys.begin(), Input::CreateDevice<Input::ButtonDevice>);
    std::transform(Settings::values.keyboard_mods.begin(), Settings::values.keyboard_mods.end(),
                   keyboard_mods.begin(), Input::CreateDevice<Input::ButtonDevice>);
}
} // namespace Service::HID
