// Copyright 2020 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/input.h"

namespace InputCommon {

/**
 * A touch device factory that takes a list of button devices and combines them into a touch device.
 */
class TouchFromButton final : public Common::Input::Factory<Common::Input::InputDevice> {
public:
    /**
     * Creates a touch device from a list of button devices
     */
    std::unique_ptr<Common::Input::InputDevice> Create(const Common::ParamPackage& params) override;
};

} // namespace InputCommon
