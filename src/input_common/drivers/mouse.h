// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

#include <stop_token>
#include <thread>

#include "common/vector_math.h"
#include "input_common/input_engine.h"

namespace InputCommon {

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

/**
 * A button device factory representing a keyboard. It receives keyboard events and forward them
 * to all button devices it created.
 */
class Mouse final : public InputEngine {
public:
    explicit Mouse(std::string input_engine_);

    /**
     * Signals that mouse has moved.
     * @param x the x-coordinate of the cursor
     * @param y the y-coordinate of the cursor
     * @param center_x the x-coordinate of the middle of the screen
     * @param center_y the y-coordinate of the middle of the screen
     */
    void MouseMove(int x, int y, f32 touch_x, f32 touch_y, int center_x, int center_y);

    /**
     * Sets the status of all buttons bound with the key to pressed
     * @param key_code the code of the key to press
     */
    void PressButton(int x, int y, f32 touch_x, f32 touch_y, MouseButton button);

    /**
     * Sets the status of all buttons bound with the key to released
     * @param key_code the code of the key to release
     */
    void ReleaseButton(MouseButton button);

    /**
     * Sets the status of the mouse wheel
     * @param x delta movement in the x direction
     * @param y delta movement in the y direction
     */
    void MouseWheelChange(int x, int y);

    void ReleaseAllButtons();

    std::vector<Common::ParamPackage> GetInputDevices() const override;
    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage& params) override;
    Common::Input::ButtonNames GetUIName(const Common::ParamPackage& params) const override;

private:
    void UpdateThread(std::stop_token stop_token);
    void StopPanning();

    Common::Input::ButtonNames GetUIButtonName(const Common::ParamPackage& params) const;

    Common::Vec2<int> mouse_origin;
    Common::Vec2<int> last_mouse_position;
    Common::Vec2<float> last_mouse_change;
    Common::Vec2<int> wheel_position;
    bool button_pressed;
    int mouse_panning_timout{};
    std::jthread update_thread;
};

} // namespace InputCommon
