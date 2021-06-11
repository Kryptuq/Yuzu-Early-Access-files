// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
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

Controller_Gesture::Controller_Gesture(Core::System& system_) : ControllerBase(system_) {}
Controller_Gesture::~Controller_Gesture() = default;

void Controller_Gesture::OnInit() {
    for (std::size_t id = 0; id < MAX_FINGERS; ++id) {
        mouse_finger_id[id] = MAX_POINTS;
        keyboard_finger_id[id] = MAX_POINTS;
        udp_finger_id[id] = MAX_POINTS;
    }
    shared_memory.header.entry_count = 0;
    force_update = true;
}

void Controller_Gesture::OnRelease() {}

void Controller_Gesture::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                  std::size_t size) {
    shared_memory.header.timestamp = core_timing.GetCPUTicks();
    shared_memory.header.total_entry_count = 17;

    if (!IsControllerActivated()) {
        shared_memory.header.entry_count = 0;
        shared_memory.header.last_entry_index = 0;
        return;
    }

    ReadTouchInput();

    GestureProperties gesture = GetGestureProperties();
    f32 time_difference = static_cast<f32>(shared_memory.header.timestamp - last_update_timestamp) /
                          (1000 * 1000 * 1000);

    // Only update if necesary
    if (!ShouldUpdateGesture(gesture, time_difference)) {
        return;
    }

    last_update_timestamp = shared_memory.header.timestamp;
    UpdateGestureSharedMemory(data, size, gesture, time_difference);
}

void Controller_Gesture::ReadTouchInput() {
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
}

bool Controller_Gesture::ShouldUpdateGesture(const GestureProperties& gesture,
                                             f32 time_difference) {
    const auto& last_entry = shared_memory.gesture_states[shared_memory.header.last_entry_index];
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
    if (last_entry.type == TouchType::Touch && last_entry.point_count == 1 &&
        time_difference > press_delay) {
        return enable_press_and_tap;
    }

    return false;
}

void Controller_Gesture::UpdateGestureSharedMemory(u8* data, std::size_t size,
                                                   GestureProperties& gesture,
                                                   f32 time_difference) {
    TouchType type = TouchType::Idle;
    Attribute attributes{};

    const auto& last_entry = shared_memory.gesture_states[shared_memory.header.last_entry_index];
    shared_memory.header.last_entry_index = (shared_memory.header.last_entry_index + 1) % 17;
    auto& cur_entry = shared_memory.gesture_states[shared_memory.header.last_entry_index];

    if (shared_memory.header.entry_count < 16) {
        shared_memory.header.entry_count++;
    }

    cur_entry.sampling_number = last_entry.sampling_number + 1;
    cur_entry.sampling_number2 = cur_entry.sampling_number;

    // Reset values to default
    cur_entry.delta = {};
    cur_entry.vel_x = 0;
    cur_entry.vel_y = 0;
    cur_entry.direction = Direction::None;
    cur_entry.rotation_angle = 0;
    cur_entry.scale = 0;

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
    cur_entry.detection_count = gesture.detection_count;
    cur_entry.type = type;
    cur_entry.attributes = attributes;
    cur_entry.pos = gesture.mid_point;
    cur_entry.point_count = static_cast<s32>(gesture.active_points);
    cur_entry.points = gesture.points;
    last_gesture = gesture;

    std::memcpy(data + SHARED_MEMORY_OFFSET, &shared_memory, sizeof(SharedMemory));
}

void Controller_Gesture::NewGesture(GestureProperties& gesture, TouchType& type,
                                    Attribute& attributes) {
    const auto& last_entry = GetLastGestureEntry();

    gesture.detection_count++;
    type = TouchType::Touch;

    // New touch after cancel is not considered new
    if (last_entry.type != TouchType::Cancel) {
        attributes.is_new_touch.Assign(1);
        enable_press_and_tap = true;
    }
}

void Controller_Gesture::UpdateExistingGesture(GestureProperties& gesture, TouchType& type,
                                               f32 time_difference) {
    const auto& last_entry = GetLastGestureEntry();

    // Promote to pan type if touch moved
    for (size_t id = 0; id < MAX_POINTS; id++) {
        if (gesture.points[id] != last_gesture.points[id]) {
            type = TouchType::Pan;
            break;
        }
    }

    // Number of fingers changed cancel the last event and clear data
    if (gesture.active_points != last_gesture.active_points) {
        type = TouchType::Cancel;
        enable_press_and_tap = false;
        gesture.active_points = 0;
        gesture.mid_point = {};
        gesture.points.fill({});
        return;
    }

    // Calculate extra parameters of panning
    if (type == TouchType::Pan) {
        UpdatePanEvent(gesture, last_gesture, type, time_difference);
        return;
    }

    // Promote to press type
    if (last_entry.type == TouchType::Touch) {
        type = TouchType::Press;
    }
}

void Controller_Gesture::EndGesture(GestureProperties& gesture,
                                    GestureProperties& last_gesture_props, TouchType& type,
                                    Attribute& attributes, f32 time_difference) {
    const auto& last_entry = GetLastGestureEntry();

    if (last_gesture_props.active_points != 0) {
        switch (last_entry.type) {
        case TouchType::Touch:
            if (enable_press_and_tap) {
                SetTapEvent(gesture, last_gesture_props, type, attributes);
                return;
            }
            type = TouchType::Cancel;
            force_update = true;
            break;
        case TouchType::Press:
        case TouchType::Tap:
        case TouchType::Swipe:
        case TouchType::Pinch:
        case TouchType::Rotate:
            type = TouchType::Complete;
            force_update = true;
            break;
        case TouchType::Pan:
            EndPanEvent(gesture, last_gesture_props, type, time_difference);
            break;
        default:
            break;
        }
        return;
    }
    if (last_entry.type == TouchType::Complete || last_entry.type == TouchType::Cancel) {
        gesture.detection_count++;
    }
}

void Controller_Gesture::SetTapEvent(GestureProperties& gesture,
                                     GestureProperties& last_gesture_props, TouchType& type,
                                     Attribute& attributes) {
    type = TouchType::Tap;
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
                                        GestureProperties& last_gesture_props, TouchType& type,
                                        f32 time_difference) {
    auto& cur_entry = shared_memory.gesture_states[shared_memory.header.last_entry_index];
    const auto& last_entry = GetLastGestureEntry();

    cur_entry.delta = gesture.mid_point - last_entry.pos;
    cur_entry.vel_x = static_cast<f32>(cur_entry.delta.x) / time_difference;
    cur_entry.vel_y = static_cast<f32>(cur_entry.delta.y) / time_difference;
    last_pan_time_difference = time_difference;

    // Promote to pinch type
    if (std::abs(gesture.average_distance - last_gesture_props.average_distance) >
        pinch_threshold) {
        type = TouchType::Pinch;
        cur_entry.scale = gesture.average_distance / last_gesture_props.average_distance;
    }

    const f32 angle_between_two_lines = std::atan((gesture.angle - last_gesture_props.angle) /
                                                  (1 + (gesture.angle * last_gesture_props.angle)));
    // Promote to rotate type
    if (std::abs(angle_between_two_lines) > angle_threshold) {
        type = TouchType::Rotate;
        cur_entry.scale = 0;
        cur_entry.rotation_angle = angle_between_two_lines * 180.0f / Common::PI;
    }
}

void Controller_Gesture::EndPanEvent(GestureProperties& gesture,
                                     GestureProperties& last_gesture_props, TouchType& type,
                                     f32 time_difference) {
    auto& cur_entry = shared_memory.gesture_states[shared_memory.header.last_entry_index];
    const auto& last_entry = GetLastGestureEntry();
    cur_entry.vel_x =
        static_cast<f32>(last_entry.delta.x) / (last_pan_time_difference + time_difference);
    cur_entry.vel_y =
        static_cast<f32>(last_entry.delta.y) / (last_pan_time_difference + time_difference);
    const f32 curr_vel =
        std::sqrt((cur_entry.vel_x * cur_entry.vel_x) + (cur_entry.vel_y * cur_entry.vel_y));

    // Set swipe event with parameters
    if (curr_vel > swipe_threshold) {
        SetSwipeEvent(gesture, last_gesture_props, type);
        return;
    }

    // End panning without swipe
    type = TouchType::Complete;
    cur_entry.vel_x = 0;
    cur_entry.vel_y = 0;
    force_update = true;
}

void Controller_Gesture::SetSwipeEvent(GestureProperties& gesture,
                                       GestureProperties& last_gesture_props, TouchType& type) {
    auto& cur_entry = shared_memory.gesture_states[shared_memory.header.last_entry_index];
    const auto& last_entry = GetLastGestureEntry();

    type = TouchType::Swipe;
    gesture = last_gesture_props;
    force_update = true;
    cur_entry.delta = last_entry.delta;

    if (std::abs(cur_entry.delta.x) > std::abs(cur_entry.delta.y)) {
        if (cur_entry.delta.x > 0) {
            cur_entry.direction = Direction::Right;
            return;
        }
        cur_entry.direction = Direction::Left;
        return;
    }
    if (cur_entry.delta.y > 0) {
        cur_entry.direction = Direction::Down;
        return;
    }
    cur_entry.direction = Direction::Up;
}

void Controller_Gesture::OnLoadInputDevices() {
    touch_mouse_device = Input::CreateDevice<Input::TouchDevice>("engine:emu_window");
    touch_udp_device = Input::CreateDevice<Input::TouchDevice>("engine:cemuhookudp");
    touch_btn_device = Input::CreateDevice<Input::TouchDevice>("engine:touch_from_button");
}

std::optional<std::size_t> Controller_Gesture::GetUnusedFingerID() const {
    // Dont assign any touch input to a point if disabled
    if (!Settings::values.touchscreen.enabled) {
        return std::nullopt;
    }
    std::size_t first_free_id = 0;
    while (first_free_id < MAX_POINTS) {
        if (!fingers[first_free_id].pressed) {
            return first_free_id;
        } else {
            first_free_id++;
        }
    }
    return std::nullopt;
}

Controller_Gesture::GestureState& Controller_Gesture::GetLastGestureEntry() {
    return shared_memory.gesture_states[(shared_memory.header.last_entry_index + 16) % 17];
}

const Controller_Gesture::GestureState& Controller_Gesture::GetLastGestureEntry() const {
    return shared_memory.gesture_states[(shared_memory.header.last_entry_index + 16) % 17];
}

std::size_t Controller_Gesture::UpdateTouchInputEvent(
    const std::tuple<float, float, bool>& touch_input, std::size_t finger_id) {
    const auto& [x, y, pressed] = touch_input;
    if (finger_id > MAX_POINTS) {
        LOG_ERROR(Service_HID, "Invalid finger id {}", finger_id);
        return MAX_POINTS;
    }
    if (pressed) {
        if (finger_id == MAX_POINTS) {
            const auto first_free_id = GetUnusedFingerID();
            if (!first_free_id) {
                // Invalid finger id do nothing
                return MAX_POINTS;
            }
            finger_id = first_free_id.value();
            fingers[finger_id].pressed = true;
        }
        fingers[finger_id].pos = {x, y};
        return finger_id;
    }

    if (finger_id != MAX_POINTS) {
        fingers[finger_id].pressed = false;
    }

    return MAX_POINTS;
}

Controller_Gesture::GestureProperties Controller_Gesture::GetGestureProperties() {
    GestureProperties gesture;
    std::array<Finger, MAX_POINTS> active_fingers;
    const auto end_iter = std::copy_if(fingers.begin(), fingers.end(), active_fingers.begin(),
                                       [](const auto& finger) { return finger.pressed; });
    gesture.active_points =
        static_cast<std::size_t>(std::distance(active_fingers.begin(), end_iter));

    for (size_t id = 0; id < gesture.active_points; ++id) {
        const auto& [active_x, active_y] = active_fingers[id].pos;
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
