// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/point.h"
#include "core/frontend/input.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_Gesture final : public ControllerBase {
public:
    explicit Controller_Gesture(Core::System& system_);
    ~Controller_Gesture() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

private:
    static constexpr size_t MAX_FINGERS = 16;
    static constexpr size_t MAX_POINTS = 4;

    enum class TouchType : u32 {
        Idle,     // Nothing touching the screen
        Complete, // Set at the end of a touch event
        Cancel,   // Set when the number of fingers change
        Touch,    // A finger just touched the screen
        Press,    // Set if last type is touch and the finger hasn't moved
        Tap,      // Fast press then release
        Pan,      // All points moving together across the screen
        Swipe,    // Fast press movement and release of a single point
        Pinch,    // All points moving away/closer to the midpoint
        Rotate,   // All points rotating from the midpoint
    };

    enum class Direction : u32 {
        None,
        Left,
        Up,
        Right,
        Down,
    };

    struct Attribute {
        union {
            u32_le raw{};

            BitField<4, 1, u32> is_new_touch;
            BitField<8, 1, u32> is_double_tap;
        };
    };
    static_assert(sizeof(Attribute) == 4, "Attribute is an invalid size");

    struct GestureState {
        s64_le sampling_number;
        s64_le sampling_number2;
        s64_le detection_count;
        TouchType type;
        Direction direction;
        Common::Point<s32_le> pos;
        Common::Point<s32_le> delta;
        f32 vel_x;
        f32 vel_y;
        Attribute attributes;
        f32 scale;
        f32 rotation_angle;
        s32_le point_count;
        std::array<Common::Point<s32_le>, 4> points;
    };
    static_assert(sizeof(GestureState) == 0x68, "GestureState is an invalid size");

    struct SharedMemory {
        CommonHeader header;
        std::array<GestureState, 17> gesture_states;
    };
    static_assert(sizeof(SharedMemory) == 0x708, "SharedMemory is an invalid size");

    struct Finger {
        Common::Point<f32> pos{};
        bool pressed{};
    };

    struct GestureProperties {
        std::array<Common::Point<s32_le>, MAX_POINTS> points{};
        std::size_t active_points{};
        Common::Point<s32_le> mid_point{};
        s64_le detection_count{};
        u64_le delta_time{};
        f32 average_distance{};
        f32 angle{};
    };

    // Reads input from all available input engines
    void ReadTouchInput();

    // Returns true if gesture state needs to be updated
    bool ShouldUpdateGesture(const GestureProperties& gesture, f32 time_difference);

    // Updates the shared memory to the next state
    void UpdateGestureSharedMemory(u8* data, std::size_t size, GestureProperties& gesture,
                                   f32 time_difference);

    // Initializes new gesture
    void NewGesture(GestureProperties& gesture, TouchType& type, Attribute& attributes);

    // Updates existing gesture state
    void UpdateExistingGesture(GestureProperties& gesture, TouchType& type, f32 time_difference);

    // Terminates exiting gesture
    void EndGesture(GestureProperties& gesture, GestureProperties& last_gesture_props,
                    TouchType& type, Attribute& attributes, f32 time_difference);

    // Set current event to a tap event
    void SetTapEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                     TouchType& type, Attribute& attributes);

    // Calculates and set the extra parameters related to a pan event
    void UpdatePanEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                        TouchType& type, f32 time_difference);

    // Terminates the pan event
    void EndPanEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                     TouchType& type, f32 time_difference);

    // Set current event to a swipe event
    void SetSwipeEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                       TouchType& type);

    // Returns an unused finger id, if there is no fingers available std::nullopt is returned.
    [[nodiscard]] std::optional<size_t> GetUnusedFingerID() const;

    // Retrieves the last gesture entry, as indicated by shared memory indices.
    [[nodiscard]] GestureState& GetLastGestureEntry();
    [[nodiscard]] const GestureState& GetLastGestureEntry() const;

    /**
     * If the touch is new it tries to assign a new finger id, if there is no fingers available no
     * changes will be made. Updates the coordinates if the finger id it's already set. If the touch
     * ends delays the output by one frame to set the end_touch flag before finally freeing the
     * finger id
     */
    size_t UpdateTouchInputEvent(const std::tuple<float, float, bool>& touch_input,
                                 size_t finger_id);

    // Returns the average distance, angle and middle point of the active fingers
    GestureProperties GetGestureProperties();

    SharedMemory shared_memory{};
    std::unique_ptr<Input::TouchDevice> touch_mouse_device;
    std::unique_ptr<Input::TouchDevice> touch_udp_device;
    std::unique_ptr<Input::TouchDevice> touch_btn_device;
    std::array<size_t, MAX_FINGERS> mouse_finger_id{};
    std::array<size_t, MAX_FINGERS> keyboard_finger_id{};
    std::array<size_t, MAX_FINGERS> udp_finger_id{};
    std::array<Finger, MAX_POINTS> fingers{};
    GestureProperties last_gesture{};
    s64_le last_update_timestamp{};
    s64_le last_tap_timestamp{};
    f32 last_pan_time_difference{};
    bool force_update{false};
    bool enable_press_and_tap{false};
};
} // namespace Service::HID
