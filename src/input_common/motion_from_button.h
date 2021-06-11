// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/frontend/input.h"

namespace InputCommon {

/**
 * An motion device factory that takes a keyboard button and uses it as a random
 * motion device.
 */
class MotionFromButton final : public Input::Factory<Input::MotionDevice> {
public:
    /**
     * Creates an motion device from button devices
     * @param params contains parameters for creating the device:
     *     - "key": a serialized ParamPackage for creating a button device
     */
    std::unique_ptr<Input::MotionDevice> Create(const Common::ParamPackage& params) override;
};

} // namespace InputCommon
