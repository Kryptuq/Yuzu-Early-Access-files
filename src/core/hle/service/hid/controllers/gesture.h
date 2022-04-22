// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/point.h"
#include "core/hid/emulated_console.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Service::HID {
class Controller_Gesture final : public ControllerBase {
public:
    explicit Controller_Gesture(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_);
    ~Controller_Gesture() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

private:
    static constexpr size_t MAX_FINGERS = 16;
    static constexpr size_t MAX_POINTS = 4;

    // This is nn::hid::GestureType
    enum class GestureType : u32 {
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

    // This is nn::hid::GestureDirection
    enum class GestureDirection : u32 {
        None,
        Left,
        Up,
        Right,
        Down,
    };

    // This is nn::hid::GestureAttribute
    struct GestureAttribute {
        union {
            u32 raw{};

            BitField<4, 1, u32> is_new_touch;
            BitField<8, 1, u32> is_double_tap;
        };
    };
    static_assert(sizeof(GestureAttribute) == 4, "GestureAttribute is an invalid size");

    // This is nn::hid::GestureState
    struct GestureState {
        s64 sampling_number{};
        s64 detection_count{};
        GestureType type{GestureType::Idle};
        GestureDirection direction{GestureDirection::None};
        Common::Point<s32> pos{};
        Common::Point<s32> delta{};
        f32 vel_x{};
        f32 vel_y{};
        GestureAttribute attributes{};
        f32 scale{};
        f32 rotation_angle{};
        s32 point_count{};
        std::array<Common::Point<s32>, 4> points{};
    };
    static_assert(sizeof(GestureState) == 0x60, "GestureState is an invalid size");

    struct GestureProperties {
        std::array<Common::Point<s32>, MAX_POINTS> points{};
        std::size_t active_points{};
        Common::Point<s32> mid_point{};
        s64 detection_count{};
        u64 delta_time{};
        f32 average_distance{};
        f32 angle{};
    };

    struct GestureSharedMemory {
        // This is nn::hid::detail::GestureLifo
        Lifo<GestureState, hid_entry_count> gesture_lifo{};
        static_assert(sizeof(gesture_lifo) == 0x708, "gesture_lifo is an invalid size");
        INSERT_PADDING_WORDS(0x3E);
    };
    static_assert(sizeof(GestureSharedMemory) == 0x800, "GestureSharedMemory is an invalid size");

    // Reads input from all available input engines
    void ReadTouchInput();

    // Returns true if gesture state needs to be updated
    bool ShouldUpdateGesture(const GestureProperties& gesture, f32 time_difference);

    // Updates the shared memory to the next state
    void UpdateGestureSharedMemory(u8* data, std::size_t size, GestureProperties& gesture,
                                   f32 time_difference);

    // Initializes new gesture
    void NewGesture(GestureProperties& gesture, GestureType& type, GestureAttribute& attributes);

    // Updates existing gesture state
    void UpdateExistingGesture(GestureProperties& gesture, GestureType& type, f32 time_difference);

    // Terminates exiting gesture
    void EndGesture(GestureProperties& gesture, GestureProperties& last_gesture_props,
                    GestureType& type, GestureAttribute& attributes, f32 time_difference);

    // Set current event to a tap event
    void SetTapEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                     GestureType& type, GestureAttribute& attributes);

    // Calculates and set the extra parameters related to a pan event
    void UpdatePanEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                        GestureType& type, f32 time_difference);

    // Terminates the pan event
    void EndPanEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                     GestureType& type, f32 time_difference);

    // Set current event to a swipe event
    void SetSwipeEvent(GestureProperties& gesture, GestureProperties& last_gesture_props,
                       GestureType& type);

    // Retrieves the last gesture entry, as indicated by shared memory indices.
    [[nodiscard]] const GestureState& GetLastGestureEntry() const;

    // Returns the average distance, angle and middle point of the active fingers
    GestureProperties GetGestureProperties();

    GestureState next_state{};
    GestureSharedMemory* shared_memory = nullptr;
    Core::HID::EmulatedConsole* console = nullptr;

    std::array<Core::HID::TouchFinger, MAX_POINTS> fingers{};
    GestureProperties last_gesture{};
    s64 last_update_timestamp{};
    s64 last_tap_timestamp{};
    f32 last_pan_time_difference{};
    bool force_update{false};
    bool enable_press_and_tap{false};
};
} // namespace Service::HID
