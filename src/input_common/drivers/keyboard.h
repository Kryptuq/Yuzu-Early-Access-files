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
class Keyboard final : public InputEngine {
public:
    explicit Keyboard(std::string input_engine_);

    /**
     * Sets the status of all buttons bound with the key to pressed
     * @param key_code the code of the key to press
     */
    void PressKey(int key_code);

    /**
     * Sets the status of all buttons bound with the key to released
     * @param key_code the code of the key to release
     */
    void ReleaseKey(int key_code);

    /**
     * Sets the status of the keyboard key to pressed
     * @param key_index index of the key to press
     */
    void PressKeyboardKey(int key_index);

    /**
     * Sets the status of the keyboard key to released
     * @param key_index index of the key to release
     */
    void ReleaseKeyboardKey(int key_index);

    /**
     * Sets the status of all keyboard modifier keys
     * @param key_modifiers the code of the key to release
     */
    void SetKeyboardModifiers(int key_modifiers);

    /// Sets all keys to the non pressed state
    void ReleaseAllKeys();

    /// Used for automapping features
    std::vector<Common::ParamPackage> GetInputDevices() const override;
};

} // namespace InputCommon
