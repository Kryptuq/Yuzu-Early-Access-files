// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/gesture.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3BA00;

// HW is around 700, value is set to 400 to make it easier to trigger with mouse
constexpr f32 swipe_threshold = 400.0f; // Threshold in pixels/s
constexpr f32 angle_threshold = 0.015f; // Threshold in radians
constexpr f32 pinch_threshold = 0.5f;   // Threshold in pixels
constexpr f32 press_delay = 0.5f;       // Time in seconds
constexpr f32 double_tap_delay = 0.35f; // Time in seconds

constexpr f32 Square(s32 num) {
    return static_cast<f32>(num * num);
}

Controller_Gesture::Controller_Gesture(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_)
    : ControllerBase(hid_core_) {
    static_assert(SHARED_MEMORY_OFFSET + sizeof(GestureSharedMemory) < shared_memory_size,
                  "GestureSharedMemory is bigger than the shared memory");
    shared_memory = std::construct_at(
        reinterpret_cast<GestureSharedMemory*>(raw_shared_memory_ + SHARED_MEMORY_OFFSET));
    console = hid_core.GetEmulatedConsole();
}
Controller_Gesture::~Controller_Gesture() = default;

void Controller_Gesture::OnInit() {
    shared_memory->gesture_lifo.buffer_count = 0;
    shared_memory->gesture_lifo.buffer_tail = 0;
    force_update = true;
}

void Controller_Gesture::OnRelease() {}

void Controller_Gesture::OnUpdate(const Core::Timing::CoreTiming& core_timing) {
    if (!IsControllerActivated()) {
        shared_memory->gesture_lifo.buffer_count = 0;
        shared_memory->gesture_lifo.buffer_tail = 0;
        return;
    }

    ReadTouchInput();

    GestureProperties gesture = GetGestureProperties();
    f32 time_difference =
        static_cast<f32>(shared_memory->gesture_lifo.timestamp - last_update_timestamp) /
        (1000 * 1000 * 1000);

    // Only update if necessary
    if (!ShouldUpdateGesture(gesture, time_difference)) {
        return;
    }

    last_update_timestamp = shared_memory->gesture_lifo.timestamp;
    UpdateGestureSharedMemory(gesture, time_difference);
}

void Controller_Gesture::ReadTouchInput() {
    if (!Settings::values.touchscreen.enabled) {
        fingers = {};
        return;
    }

    const auto touch_status = console->GetTouch();
    for (std::size_t id = 0; id < fingers.size(); ++id) {
        fingers[id] = touch_status[id];
    }
}

bool Controller_Gesture::ShouldUpdateGesture(const GestureProperties& gesture,
                                             f32 time_difference) {
    const auto& last_entry = GetLastGestureEntry();
    if (force_update) {
        force_update = false;
        return true;
    }

    // Update if coordinates change
    for (size_t id = 0; id < MAX_POINTS; id++) {
        if (gesture.points[id] != last_gesture.points[id]) {
            return true;
        }
    }

    // Update on press and hold event after 0.5 seconds
    if (last_entry.type == GestureType::Touch && last_entry.point_count == 1 &&
        time_difference > press_delay) {
        return enable_press_and_tap;
    }

    return false;
}

void Controller_Gesture::UpdateGestureSharedMemory(GestureProperties& gesture,
                                                   f32 time_difference) {
    GestureType type = GestureType::Idle;
    GestureAttribute attributes{};

    const auto& last_entry = shared_memory->gesture_lifo.ReadCurrentEntry().state;

    // Reset next state to default
    next_state.sampling_number = last_entry.sampling_number + 1;
    next_state.delta = {};
    next_state.vel_x = 0;
    next_state.vel_y = 0;
    next_state.direction = GestureDirection::None;
    next_state.rotation_angle = 0;
    next_state.scale = 0;

    if (gesture.active_points > 0) {
        if (last_gesture.active_points == 0) {
            NewGesture(gesture, type, attributes);
        } else {
            UpdateExistingGesture(gesture, type, time_difference);
        }
    } else {
        EndGesture(gesture, last_gesture, type, attributes, time_difference);
    }

    // Apply attributes
    next_state.detection_count = gesture.detection_count;
    next_state.type = type;
    next_state.attributes = attributes;
    next_state.pos = gesture.mid_point;
    next_state.point_count = static_cast<s32>(gesture.active_points);
    next_state.points = gesture.points;
    last_gesture = gesture;

    shared_memory->gesture_lifo.WriteNextEntry(next_state);
}

void Controller_Gesture::NewGesture(GestureProperties& gesture, GestureType& type,
                                    GestureAttribute& attributes) {
    const auto& last_entry = GetLastGestureEntry();

    gesture.detection_count++;
    type = GestureType::Touch;

    // New touch after cancel is not considered new
    if (last_entry.type != GestureType::Cancel) {
        attributes.is_new_touch.Assign(1);
        enable_press_and_tap = true;
    }
}

void Controller_Gesture::UpdateExistingGesture(GestureProperties& gesture, GestureType& type,
                                               f32 time_difference) {
    const auto& last_entry = GetLastGestureEntry();

    // Promote to pan type if touch moved
    for (size_t id = 0; id < MAX_POINTS; id++) {
        if (gesture.points[id] != last_gesture.points[id]) {
            type = GestureType::Pan;
            break;
        }
    }

    // Number of fingers changed cancel the last event and clear data
    if (gesture.active_points != last_gesture.active_points) {
        type = GestureType::Cancel;
        enable_press_and_tap = false;
        gesture.active_points = 0;
        gesture.mid_point = {};
        gesture.points.fill({});
        return;
    }

    // Calculate extra parameters of panning
    if (type == GestureType::Pan) {
        UpdatePanEvent(gesture, last_gesture, type, time_difference);
        return;
    }

    // Promote to press type
    if (last_entry.type == GestureType::Touch) {
        type = GestureType::Press;
    }
}

void Controller_Gesture::EndGesture(GestureProperties& gesture,
                                    GestureProperties& last_gesture_props, GestureType& type,
                                    GestureAttribute& attributes, f32 time_difference) {
    const auto& last_entry = GetLastGestureEntry();

    if (last_gesture_props.active_points != 0) {
        switch (last_entry.type) {
        case GestureType::Touch:
            if (enable_press_and_tap) {
                SetTapEvent(gesture, last_gesture_props, type, attributes);
                return;
            }
            type = GestureType::Cancel;
            force_update = true;
            break;
        case GestureType::Press:
        case GestureType::Tap:
        case GestureType::Swipe:
        case GestureType::Pinch:
        case GestureType::Rotate:
            type = GestureType::Complete;
            force_update = true;
            break;
        case GestureType::Pan:
            EndPanEvent(gesture, last_gesture_props, type, time_difference);
            break;
        default:
            break;
        }
        return;
    }
    if (last_entry.type == GestureType::Complete || last_entry.type == GestureType::Cancel) {
        gesture.detection_count++;
    }
}

void Controller_Gesture::SetTapEvent(GestureProperties& gesture,
                                     GestureProperties& last_gesture_props, GestureType& type,
                                     GestureAttribute& attributes) {
    type = GestureType::Tap;
    gesture = last_gesture_props;
    force_update = true;
    f32 tap_time_difference =
        static_cast<f32>(last_update_timestamp - last_tap_timestamp) / (1000 * 1000 * 1000);
    last_tap_timestamp = last_update_timestamp;
    if (tap_time_difference < double_tap_delay) {
        attributes.is_double_tap.Assign(1);
    }
}

void Controller_Gesture::UpdatePanEvent(GestureProperties& gesture,
                                        GestureProperties& last_gesture_props, GestureType& type,
                                        f32 time_difference) {
    const auto& last_entry = GetLastGestureEntry();

    next_state.delta = gesture.mid_point - last_entry.pos;
    next_state.vel_x = static_cast<f32>(next_state.delta.x) / time_difference;
    next_state.vel_y = static_cast<f32>(next_state.delta.y) / time_difference;
    last_pan_time_difference = time_difference;

    // Promote to pinch type
    if (std::abs(gesture.average_distance - last_gesture_props.average_distance) >
        pinch_threshold) {
        type = GestureType::Pinch;
        next_state.scale = gesture.average_distance / last_gesture_props.average_distance;
    }

    const f32 angle_between_two_lines = std::atan((gesture.angle - last_gesture_props.angle) /
                                                  (1 + (gesture.angle * last_gesture_props.angle)));
    // Promote to rotate type
    if (std::abs(angle_between_two_lines) > angle_threshold) {
        type = GestureType::Rotate;
        next_state.scale = 0;
        next_state.rotation_angle = angle_between_two_lines * 180.0f / Common::PI;
    }
}

void Controller_Gesture::EndPanEvent(GestureProperties& gesture,
                                     GestureProperties& last_gesture_props, GestureType& type,
                                     f32 time_difference) {
    const auto& last_entry = GetLastGestureEntry();
    next_state.vel_x =
        static_cast<f32>(last_entry.delta.x) / (last_pan_time_difference + time_difference);
    next_state.vel_y =
        static_cast<f32>(last_entry.delta.y) / (last_pan_time_difference + time_difference);
    const f32 curr_vel =
        std::sqrt((next_state.vel_x * next_state.vel_x) + (next_state.vel_y * next_state.vel_y));

    // Set swipe event with parameters
    if (curr_vel > swipe_threshold) {
        SetSwipeEvent(gesture, last_gesture_props, type);
        return;
    }

    // End panning without swipe
    type = GestureType::Complete;
    next_state.vel_x = 0;
    next_state.vel_y = 0;
    force_update = true;
}

void Controller_Gesture::SetSwipeEvent(GestureProperties& gesture,
                                       GestureProperties& last_gesture_props, GestureType& type) {
    const auto& last_entry = GetLastGestureEntry();

    type = GestureType::Swipe;
    gesture = last_gesture_props;
    force_update = true;
    next_state.delta = last_entry.delta;

    if (std::abs(next_state.delta.x) > std::abs(next_state.delta.y)) {
        if (next_state.delta.x > 0) {
            next_state.direction = GestureDirection::Right;
            return;
        }
        next_state.direction = GestureDirection::Left;
        return;
    }
    if (next_state.delta.y > 0) {
        next_state.direction = GestureDirection::Down;
        return;
    }
    next_state.direction = GestureDirection::Up;
}

const Controller_Gesture::GestureState& Controller_Gesture::GetLastGestureEntry() const {
    return shared_memory->gesture_lifo.ReadCurrentEntry().state;
}

Controller_Gesture::GestureProperties Controller_Gesture::GetGestureProperties() {
    GestureProperties gesture;
    std::array<Core::HID::TouchFinger, MAX_POINTS> active_fingers;
    const auto end_iter = std::copy_if(fingers.begin(), fingers.end(), active_fingers.begin(),
                                       [](const auto& finger) { return finger.pressed; });
    gesture.active_points =
        static_cast<std::size_t>(std::distance(active_fingers.begin(), end_iter));

    for (size_t id = 0; id < gesture.active_points; ++id) {
        const auto& [active_x, active_y] = active_fingers[id].position;
        gesture.points[id] = {
            .x = static_cast<s32>(active_x * Layout::ScreenUndocked::Width),
            .y = static_cast<s32>(active_y * Layout::ScreenUndocked::Height),
        };

        // Hack: There is no touch in docked but games still allow it
        if (Settings::values.use_docked_mode.GetValue()) {
            gesture.points[id] = {
                .x = static_cast<s32>(active_x * Layout::ScreenDocked::Width),
                .y = static_cast<s32>(active_y * Layout::ScreenDocked::Height),
            };
        }

        gesture.mid_point.x += static_cast<s32>(gesture.points[id].x / gesture.active_points);
        gesture.mid_point.y += static_cast<s32>(gesture.points[id].y / gesture.active_points);
    }

    for (size_t id = 0; id < gesture.active_points; ++id) {
        const f32 distance = std::sqrt(Square(gesture.mid_point.x - gesture.points[id].x) +
                                       Square(gesture.mid_point.y - gesture.points[id].y));
        gesture.average_distance += distance / static_cast<f32>(gesture.active_points);
    }

    gesture.angle = std::atan2(static_cast<f32>(gesture.mid_point.y - gesture.points[0].y),
                               static_cast<f32>(gesture.mid_point.x - gesture.points[0].x));

    gesture.detection_count = last_gesture.detection_count;

    return gesture;
}

} // namespace Service::HID
