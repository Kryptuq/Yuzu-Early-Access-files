// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/input.h"

namespace InputCommon {

/**
 * An analog device factory that takes direction button devices and combines them into a analog
 * device.
 */
class StickFromButton final : public Common::Input::Factory<Common::Input::InputDevice> {
public:
    /**
     * Creates an analog device from direction button devices
     * @param params contains parameters for creating the device:
     *     - "up": a serialized ParamPackage for creating a button device for up direction
     *     - "down": a serialized ParamPackage for creating a button device for down direction
     *     - "left": a serialized ParamPackage for creating a button device for left direction
     *     - "right": a serialized ParamPackage  for creating a button device for right direction
     *     - "modifier": a serialized ParamPackage for creating a button device as the modifier
     *     - "modifier_scale": a float for the multiplier the modifier gives to the position
     */
    std::unique_ptr<Common::Input::InputDevice> Create(const Common::ParamPackage& params) override;
};

} // namespace InputCommon
