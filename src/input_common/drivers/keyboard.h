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
class Keyboard final : public InputCommon::InputEngine {
public:
    explicit Keyboard(const std::string& input_engine_);

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

    void ReleaseAllKeys();

    /// Used for automapping features
    std::vector<Common::ParamPackage> GetInputDevices() const override;
};

} // namespace InputCommon
