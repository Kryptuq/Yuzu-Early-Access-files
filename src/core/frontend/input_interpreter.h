// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/common_types.h"

namespace Core {
class System;
}

namespace Service::HID {
class Controller_NPad;
}

enum class HIDButton : u8 {
    A,
    B,
    X,
    Y,
    LStick,
    RStick,
    L,
    R,
    ZL,
    ZR,
    Plus,
    Minus,

    DLeft,
    DUp,
    DRight,
    DDown,

    LStickLeft,
    LStickUp,
    LStickRight,
    LStickDown,

    RStickLeft,
    RStickUp,
    RStickRight,
    RStickDown,

    LeftSL,
    LeftSR,

    RightSL,
    RightSR,
};

/**
 * The InputInterpreter class interfaces with HID to retrieve button press states.
 * Input is intended to be polled every 50ms so that a button is considered to be
 * held down after 400ms has elapsed since the initial button press and subsequent
 * repeated presses occur every 50ms.
 */
class InputInterpreter {
public:
    explicit InputInterpreter(Core::System& system);
    virtual ~InputInterpreter();

    /// Gets a button state from HID and inserts it into the array of button states.
    void PollInput();

    /// Resets all the button states to their defaults.
    void ResetButtonStates();

    /**
     * Checks whether the button is pressed.
     *
     * @param button The button to check.
     *
     * @returns True when the button is pressed.
     */
    [[nodiscard]] bool IsButtonPressed(HIDButton button) const;

    /**
     * Checks whether any of the buttons in the parameter list is pressed.
     *
     * @tparam HIDButton The buttons to check.
     *
     * @returns True when at least one of the buttons is pressed.
     */
    template <HIDButton... T>
    [[nodiscard]] bool IsAnyButtonPressed() {
        return (IsButtonPressed(T) || ...);
    }

    /**
     * The specified button is considered to be pressed once
     * if it is currently pressed and not pressed previously.
     *
     * @param button The button to check.
     *
     * @returns True when the button is pressed once.
     */
    [[nodiscard]] bool IsButtonPressedOnce(HIDButton button) const;

    /**
     * Checks whether any of the buttons in the parameter list is pressed once.
     *
     * @tparam T The buttons to check.
     *
     * @returns True when at least one of the buttons is pressed once.
     */
    template <HIDButton... T>
    [[nodiscard]] bool IsAnyButtonPressedOnce() const {
        return (IsButtonPressedOnce(T) || ...);
    }

    /**
     * The specified button is considered to be held down if it is pressed in all 9 button states.
     *
     * @param button The button to check.
     *
     * @returns True when the button is held down.
     */
    [[nodiscard]] bool IsButtonHeld(HIDButton button) const;

    /**
     * Checks whether any of the buttons in the parameter list is held down.
     *
     * @tparam T The buttons to check.
     *
     * @returns True when at least one of the buttons is held down.
     */
    template <HIDButton... T>
    [[nodiscard]] bool IsAnyButtonHeld() const {
        return (IsButtonHeld(T) || ...);
    }

private:
    Service::HID::Controller_NPad& npad;

    /// Stores 9 consecutive button states polled from HID.
    std::array<u32, 9> button_states{};

    std::size_t previous_index{};
    std::size_t current_index{};
};
