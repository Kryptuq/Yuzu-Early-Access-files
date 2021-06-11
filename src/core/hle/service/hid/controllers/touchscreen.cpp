// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstring>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/input.h"
#include "core/hle/service/hid/controllers/touchscreen.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x400;

Controller_Touchscreen::Controller_Touchscreen(Core::System& system_) : ControllerBase{system_} {}
Controller_Touchscreen::~Controller_Touchscreen() = default;

void Controller_Touchscreen::OnInit() {
    for (std::size_t id = 0; id < MAX_FINGERS; ++id) {
        mouse_finger_id[id] = MAX_FINGERS;
        keyboard_finger_id[id] = MAX_FINGERS;
        udp_finger_id[id] = MAX_FINGERS;
    }
}

void Controller_Touchscreen::OnRelease() {}

void Controller_Touchscreen::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                      std::size_t size) {
    shared_memory.header.timestamp = core_timing.GetCPUTicks();
    shared_memory.header.total_entry_count = 17;

    if (!IsControllerActivated()) {
        shared_memory.header.entry_count = 0;
        shared_memory.header.last_entry_index = 0;
        return;
    }
    shared_memory.header.entry_count = 16;

    const auto& last_entry =
        shared_memory.shared_memory_entries[shared_memory.header.last_entry_index];
    shared_memory.header.last_entry_index = (shared_memory.header.last_entry_index + 1) % 17;
    auto& cur_entry = shared_memory.shared_memory_entries[shared_memory.header.last_entry_index];

    cur_entry.sampling_number = last_entry.sampling_number + 1;
    cur_entry.sampling_number2 = cur_entry.sampling_number;

    const Input::TouchStatus& mouse_status = touch_mouse_device->GetStatus();
    const Input::TouchStatus& udp_status = touch_udp_device->GetStatus();
    for (std::size_t id = 0; id < mouse_status.size(); ++id) {
        mouse_finger_id[id] = UpdateTouchInputEvent(mouse_status[id], mouse_finger_id[id]);
        udp_finger_id[id] = UpdateTouchInputEvent(udp_status[id], udp_finger_id[id]);
    }

    if (Settings::values.use_touch_from_button) {
        const Input::TouchStatus& keyboard_status = touch_btn_device->GetStatus();
        for (std::size_t id = 0; id < mouse_status.size(); ++id) {
            keyboard_finger_id[id] =
                UpdateTouchInputEvent(keyboard_status[id], keyboard_finger_id[id]);
        }
    }

    std::array<Finger, 16> active_fingers;
    const auto end_iter = std::copy_if(fingers.begin(), fingers.end(), active_fingers.begin(),
                                       [](const auto& finger) { return finger.pressed; });
    const auto active_fingers_count =
        static_cast<std::size_t>(std::distance(active_fingers.begin(), end_iter));

    const u64 tick = core_timing.GetCPUTicks();
    cur_entry.entry_count = static_cast<s32_le>(active_fingers_count);
    for (std::size_t id = 0; id < MAX_FINGERS; ++id) {
        auto& touch_entry = cur_entry.states[id];
        if (id < active_fingers_count) {
            const auto& [active_x, active_y] = active_fingers[id].position;
            touch_entry.position = {
                .x = static_cast<u16>(active_x * Layout::ScreenUndocked::Width),
                .y = static_cast<u16>(active_y * Layout::ScreenUndocked::Height),
            };
            touch_entry.diameter_x = Settings::values.touchscreen.diameter_x;
            touch_entry.diameter_y = Settings::values.touchscreen.diameter_y;
            touch_entry.rotation_angle = Settings::values.touchscreen.rotation_angle;
            touch_entry.delta_time = tick - active_fingers[id].last_touch;
            fingers[active_fingers[id].id].last_touch = tick;
            touch_entry.finger = active_fingers[id].id;
            touch_entry.attribute.raw = active_fingers[id].attribute.raw;
        } else {
            // Clear touch entry
            touch_entry.attribute.raw = 0;
            touch_entry.position = {};
            touch_entry.diameter_x = 0;
            touch_entry.diameter_y = 0;
            touch_entry.rotation_angle = 0;
            touch_entry.delta_time = 0;
            touch_entry.finger = 0;
        }
    }
    std::memcpy(data + SHARED_MEMORY_OFFSET, &shared_memory, sizeof(TouchScreenSharedMemory));
}

void Controller_Touchscreen::OnLoadInputDevices() {
    touch_mouse_device = Input::CreateDevice<Input::TouchDevice>("engine:emu_window");
    touch_udp_device = Input::CreateDevice<Input::TouchDevice>("engine:cemuhookudp");
    touch_btn_device = Input::CreateDevice<Input::TouchDevice>("engine:touch_from_button");
}

std::optional<std::size_t> Controller_Touchscreen::GetUnusedFingerID() const {
    // Dont assign any touch input to a finger if disabled
    if (!Settings::values.touchscreen.enabled) {
        return std::nullopt;
    }
    std::size_t first_free_id = 0;
    while (first_free_id < MAX_FINGERS) {
        if (!fingers[first_free_id].pressed) {
            return first_free_id;
        } else {
            first_free_id++;
        }
    }
    return std::nullopt;
}

std::size_t Controller_Touchscreen::UpdateTouchInputEvent(
    const std::tuple<float, float, bool>& touch_input, std::size_t finger_id) {
    const auto& [x, y, pressed] = touch_input;
    if (finger_id > MAX_FINGERS) {
        LOG_ERROR(Service_HID, "Invalid finger id {}", finger_id);
        return MAX_FINGERS;
    }
    if (pressed) {
        Attributes attribute{};
        if (finger_id == MAX_FINGERS) {
            const auto first_free_id = GetUnusedFingerID();
            if (!first_free_id) {
                // Invalid finger id do nothing
                return MAX_FINGERS;
            }
            finger_id = first_free_id.value();
            fingers[finger_id].pressed = true;
            fingers[finger_id].id = static_cast<u32_le>(finger_id);
            attribute.start_touch.Assign(1);
        }
        fingers[finger_id].position = {x, y};
        fingers[finger_id].attribute = attribute;
        return finger_id;
    }

    if (finger_id != MAX_FINGERS) {
        if (!fingers[finger_id].attribute.end_touch) {
            fingers[finger_id].attribute.end_touch.Assign(1);
            fingers[finger_id].attribute.start_touch.Assign(0);
            return finger_id;
        }
        fingers[finger_id].pressed = false;
    }

    return MAX_FINGERS;
}

} // namespace Service::HID
