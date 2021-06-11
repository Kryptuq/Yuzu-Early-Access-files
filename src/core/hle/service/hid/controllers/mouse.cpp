// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hle/service/hid/controllers/mouse.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3400;

Controller_Mouse::Controller_Mouse(Core::System& system_) : ControllerBase{system_} {}
Controller_Mouse::~Controller_Mouse() = default;

void Controller_Mouse::OnInit() {}
void Controller_Mouse::OnRelease() {}

void Controller_Mouse::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                std::size_t size) {
    shared_memory.header.timestamp = core_timing.GetCPUTicks();
    shared_memory.header.total_entry_count = 17;

    if (!IsControllerActivated()) {
        shared_memory.header.entry_count = 0;
        shared_memory.header.last_entry_index = 0;
        return;
    }
    shared_memory.header.entry_count = 16;

    auto& last_entry = shared_memory.mouse_states[shared_memory.header.last_entry_index];
    shared_memory.header.last_entry_index = (shared_memory.header.last_entry_index + 1) % 17;
    auto& cur_entry = shared_memory.mouse_states[shared_memory.header.last_entry_index];

    cur_entry.sampling_number = last_entry.sampling_number + 1;
    cur_entry.sampling_number2 = cur_entry.sampling_number;

    cur_entry.attribute.raw = 0;
    if (Settings::values.mouse_enabled) {
        const auto [px, py, sx, sy] = mouse_device->GetStatus();
        const auto x = static_cast<s32>(px * Layout::ScreenUndocked::Width);
        const auto y = static_cast<s32>(py * Layout::ScreenUndocked::Height);
        cur_entry.x = x;
        cur_entry.y = y;
        cur_entry.delta_x = x - last_entry.x;
        cur_entry.delta_y = y - last_entry.y;
        cur_entry.mouse_wheel_x = sx;
        cur_entry.mouse_wheel_y = sy;
        cur_entry.attribute.is_connected.Assign(1);

        using namespace Settings::NativeMouseButton;
        cur_entry.button.left.Assign(mouse_button_devices[Left]->GetStatus());
        cur_entry.button.right.Assign(mouse_button_devices[Right]->GetStatus());
        cur_entry.button.middle.Assign(mouse_button_devices[Middle]->GetStatus());
        cur_entry.button.forward.Assign(mouse_button_devices[Forward]->GetStatus());
        cur_entry.button.back.Assign(mouse_button_devices[Back]->GetStatus());
    }

    std::memcpy(data + SHARED_MEMORY_OFFSET, &shared_memory, sizeof(SharedMemory));
}

void Controller_Mouse::OnLoadInputDevices() {
    mouse_device = Input::CreateDevice<Input::MouseDevice>(Settings::values.mouse_device);
    std::transform(Settings::values.mouse_buttons.begin(), Settings::values.mouse_buttons.end(),
                   mouse_button_devices.begin(), Input::CreateDevice<Input::ButtonDevice>);
}
} // namespace Service::HID
