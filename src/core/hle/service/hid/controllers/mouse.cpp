// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hid/emulated_devices.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/mouse.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3400;

Controller_Mouse::Controller_Mouse(Core::System& system_) : ControllerBase{system_} {
    emulated_devices = system.HIDCore().GetEmulatedDevices();
}

Controller_Mouse::~Controller_Mouse() = default;

void Controller_Mouse::OnInit() {}
void Controller_Mouse::OnRelease() {}

void Controller_Mouse::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                std::size_t size) {
    if (!IsControllerActivated()) {
        mouse_lifo.buffer_count = 0;
        mouse_lifo.buffer_tail = 0;
        std::memcpy(data + SHARED_MEMORY_OFFSET, &mouse_lifo, sizeof(mouse_lifo));
        return;
    }

    const auto& last_entry = mouse_lifo.ReadCurrentEntry().state;
    next_state.sampling_number = last_entry.sampling_number + 1;

    next_state.attribute.raw = 0;
    if (Settings::values.mouse_enabled) {
        const auto& mouse_button_state = emulated_devices->GetMouseButtons();
        const auto& mouse_position_state = emulated_devices->GetMousePosition();
        next_state.attribute.is_connected.Assign(1);
        next_state.x = mouse_position_state.x;
        next_state.y = mouse_position_state.y;
        next_state.delta_x = next_state.x - last_entry.x;
        next_state.delta_y = next_state.y - last_entry.y;
        next_state.delta_wheel_x = mouse_position_state.delta_wheel_x;
        next_state.delta_wheel_y = mouse_position_state.delta_wheel_y;

        next_state.button = mouse_button_state;
    }

    mouse_lifo.WriteNextEntry(next_state);
    std::memcpy(data + SHARED_MEMORY_OFFSET, &mouse_lifo, sizeof(mouse_lifo));
}

} // namespace Service::HID
