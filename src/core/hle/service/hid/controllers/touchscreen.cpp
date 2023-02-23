// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include "common/common_types.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hid/emulated_console.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/touchscreen.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x400;

Controller_Touchscreen::Controller_Touchscreen(Core::HID::HIDCore& hid_core_,
                                               u8* raw_shared_memory_)
    : ControllerBase{hid_core_} {
    static_assert(SHARED_MEMORY_OFFSET + sizeof(TouchSharedMemory) < shared_memory_size,
                  "TouchSharedMemory is bigger than the shared memory");
    shared_memory = std::construct_at(
        reinterpret_cast<TouchSharedMemory*>(raw_shared_memory_ + SHARED_MEMORY_OFFSET));
    console = hid_core.GetEmulatedConsole();
}

Controller_Touchscreen::~Controller_Touchscreen() = default;

void Controller_Touchscreen::OnInit() {}

void Controller_Touchscreen::OnRelease() {}

void Controller_Touchscreen::OnUpdate(const Core::Timing::CoreTiming& core_timing) {
    shared_memory->touch_screen_lifo.timestamp = core_timing.GetCPUTicks();

    if (!IsControllerActivated()) {
        shared_memory->touch_screen_lifo.buffer_count = 0;
        shared_memory->touch_screen_lifo.buffer_tail = 0;
        return;
    }

    const auto touch_status = console->GetTouch();
    for (std::size_t id = 0; id < MAX_FINGERS; id++) {
        const auto& current_touch = touch_status[id];
        auto& finger = fingers[id];
        finger.id = current_touch.id;

        if (finger.attribute.start_touch) {
            finger.attribute.raw = 0;
            continue;
        }

        if (finger.attribute.end_touch) {
            finger.attribute.raw = 0;
            finger.pressed = false;
            continue;
        }

        if (!finger.pressed && current_touch.pressed) {
            // Ignore all touch fingers if disabled
            if (!Settings::values.touchscreen.enabled) {
                continue;
            }

            finger.attribute.start_touch.Assign(1);
            finger.pressed = true;
            finger.position = current_touch.position;
            continue;
        }

        if (finger.pressed && !current_touch.pressed) {
            finger.attribute.raw = 0;
            finger.attribute.end_touch.Assign(1);
            continue;
        }

        // Only update position if touch is not on a special frame
        finger.position = current_touch.position;
    }

    std::array<Core::HID::TouchFinger, MAX_FINGERS> active_fingers;
    const auto end_iter = std::copy_if(fingers.begin(), fingers.end(), active_fingers.begin(),
                                       [](const auto& finger) { return finger.pressed; });
    const auto active_fingers_count =
        static_cast<std::size_t>(std::distance(active_fingers.begin(), end_iter));

    const u64 tick = core_timing.GetCPUTicks();
    const auto& last_entry = shared_memory->touch_screen_lifo.ReadCurrentEntry().state;

    next_state.sampling_number = last_entry.sampling_number + 1;
    next_state.entry_count = static_cast<s32>(active_fingers_count);

    for (std::size_t id = 0; id < MAX_FINGERS; ++id) {
        auto& touch_entry = next_state.states[id];
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

    shared_memory->touch_screen_lifo.WriteNextEntry(next_state);
}

} // namespace Service::HID
