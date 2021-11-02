// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

#include "input_common/input_engine.h"

namespace InputCommon {

/**
 * A button device factory representing a keyboard. It receives keyboard events and forward them
 * to all button devices it created.
 */
class TouchScreen final : public InputCommon::InputEngine {
public:
    explicit TouchScreen(const std::string input_engine_);

    /**
     * Signals that mouse has moved.
     * @param x the x-coordinate of the cursor
     * @param y the y-coordinate of the cursor
     * @param center_x the x-coordinate of the middle of the screen
     * @param center_y the y-coordinate of the middle of the screen
     */
    void TouchMoved(float x, float y, std::size_t finger);

    /**
     * Sets the status of all buttons bound with the key to pressed
     * @param key_code the code of the key to press
     */
    void TouchPressed(float x, float y, std::size_t finger);

    /**
     * Sets the status of all buttons bound with the key to released
     * @param key_code the code of the key to release
     */
    void TouchReleased(std::size_t finger);

    /// Resets all inputs to their initial value
    void ReleaseAllTouch();
};

} // namespace InputCommon
