// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>

#include "input_common/input_engine.h"

namespace InputCommon {

/**
 * A touch device factory representing a touch screen. It receives touch events and forward them
 * to all touch devices it created.
 */
class TouchScreen final : public InputEngine {
public:
    explicit TouchScreen(std::string input_engine_);

    /**
     * Signals that touch has moved and marks this touch point as active
     * @param x new horizontal position
     * @param y new vertical position
     * @param finger_id of the touch point to be updated
     */
    void TouchMoved(float x, float y, std::size_t finger_id);

    /**
     * Signals and creates a new touch point with this finger id
     * @param x starting horizontal position
     * @param y starting vertical position
     * @param finger_id to be assigned to the new touch point
     */
    void TouchPressed(float x, float y, std::size_t finger_id);

    /**
     * Signals and resets the touch point related to the this finger id
     * @param finger_id to be released
     */
    void TouchReleased(std::size_t finger_id);

    /// Resets the active flag for each touch point
    void ClearActiveFlag();

    /// Releases all touch that haven't been marked as active
    void ReleaseInactiveTouch();

    /// Resets all inputs to their initial value
    void ReleaseAllTouch();

private:
    static constexpr std::size_t MAX_FINGER_COUNT = 16;

    struct TouchStatus {
        std::size_t finger_id{};
        bool is_enabled{};
        bool is_active{};
    };

    std::optional<std::size_t> GetIndexFromFingerId(std::size_t finger_id) const;

    std::optional<std::size_t> GetNextFreeIndex() const;

    std::array<TouchStatus, MAX_FINGER_COUNT> fingers{};
};

} // namespace InputCommon
