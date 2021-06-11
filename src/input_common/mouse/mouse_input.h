// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <mutex>
#include <thread>

#include "common/common_types.h"
#include "common/threadsafe_queue.h"
#include "common/vector_math.h"
#include "core/frontend/input.h"
#include "input_common/motion_input.h"

namespace MouseInput {

enum class MouseButton {
    Left,
    Right,
    Wheel,
    Backward,
    Forward,
    Task,
    Extra,
    Undefined,
};

struct MouseStatus {
    MouseButton button{MouseButton::Undefined};
};

struct MouseData {
    bool pressed{};
    std::array<int, 2> axis{};
    Input::MotionStatus motion{};
    Input::TouchStatus touch{};
};

class Mouse {
public:
    Mouse();
    ~Mouse();

    /// Used for polling
    void BeginConfiguration();
    void EndConfiguration();

    /**
     * Signals that a button is pressed.
     * @param x the x-coordinate of the cursor
     * @param y the y-coordinate of the cursor
     * @param button_ the button pressed
     */
    void PressButton(int x, int y, MouseButton button_);

    /**
     * Signals that mouse has moved.
     * @param x the x-coordinate of the cursor
     * @param y the y-coordinate of the cursor
     * @param center_x the x-coordinate of the middle of the screen
     * @param center_y the y-coordinate of the middle of the screen
     */
    void MouseMove(int x, int y, int center_x, int center_y);

    /**
     * Signals that a button is released.
     * @param button_ the button pressed
     */
    void ReleaseButton(MouseButton button_);

    /**
     * Signals that all buttons are released
     */
    void ReleaseAllButtons();

    [[nodiscard]] bool ToggleButton(std::size_t button_);
    [[nodiscard]] bool UnlockButton(std::size_t button_);

    [[nodiscard]] Common::SPSCQueue<MouseStatus>& GetMouseQueue();
    [[nodiscard]] const Common::SPSCQueue<MouseStatus>& GetMouseQueue() const;

    [[nodiscard]] MouseData& GetMouseState(std::size_t button);
    [[nodiscard]] const MouseData& GetMouseState(std::size_t button) const;

private:
    void UpdateThread();
    void UpdateYuzuSettings();
    void StopPanning();

    struct MouseInfo {
        InputCommon::MotionInput motion{0.0f, 0.0f, 0.0f};
        Common::Vec2<int> mouse_origin;
        Common::Vec2<int> last_mouse_position;
        Common::Vec2<float> last_mouse_change;
        bool is_tilting = false;
        float sensitivity{0.120f};

        float tilt_speed = 0;
        Common::Vec2<float> tilt_direction;
        MouseData data;
    };

    u16 buttons{};
    u16 toggle_buttons{};
    u16 lock_buttons{};
    std::thread update_thread;
    MouseButton last_button{MouseButton::Undefined};
    std::array<MouseInfo, 7> mouse_info;
    Common::SPSCQueue<MouseStatus> mouse_queue;
    bool configuring{false};
    bool update_thread_running{true};
    int mouse_panning_timout{};
};
} // namespace MouseInput
