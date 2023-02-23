// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/common_types.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hid/emulated_devices.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/mouse.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3400;

Controller_Mouse::Controller_Mouse(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_)
    : ControllerBase{hid_core_} {
    static_assert(SHARED_MEMORY_OFFSET + sizeof(MouseSharedMemory) < shared_memory_size,
                  "MouseSharedMemory is bigger than the shared memory");
    shared_memory = std::construct_at(
        reinterpret_cast<MouseSharedMemory*>(raw_shared_memory_ + SHARED_MEMORY_OFFSET));
    emulated_devices = hid_core.GetEmulatedDevices();
}

Controller_Mouse::~Controller_Mouse() = default;

void Controller_Mouse::OnInit() {}
void Controller_Mouse::OnRelease() {}

void Controller_Mouse::OnUpdate(const Core::Timing::CoreTiming& core_timing) {
    if (!IsControllerActivated()) {
        shared_memory->mouse_lifo.buffer_count = 0;
        shared_memory->mouse_lifo.buffer_tail = 0;
        return;
    }

    next_state = {};

    const auto& last_entry = shared_memory->mouse_lifo.ReadCurrentEntry().state;
    next_state.sampling_number = last_entry.sampling_number + 1;

    if (Settings::values.mouse_enabled) {
        const auto& mouse_button_state = emulated_devices->GetMouseButtons();
        const auto& mouse_position_state = emulated_devices->GetMousePosition();
        const auto& mouse_wheel_state = emulated_devices->GetMouseWheel();
        next_state.attribute.is_connected.Assign(1);
        next_state.x = static_cast<s32>(mouse_position_state.x * Layout::ScreenUndocked::Width);
        next_state.y = static_cast<s32>(mouse_position_state.y * Layout::ScreenUndocked::Height);
        next_state.delta_x = next_state.x - last_entry.x;
        next_state.delta_y = next_state.y - last_entry.y;
        next_state.delta_wheel_x = mouse_wheel_state.x - last_mouse_wheel_state.x;
        next_state.delta_wheel_y = mouse_wheel_state.y - last_mouse_wheel_state.y;

        last_mouse_wheel_state = mouse_wheel_state;
        next_state.button = mouse_button_state;
    }

    shared_memory->mouse_lifo.WriteNextEntry(next_state);
}

} // namespace Service::HID
