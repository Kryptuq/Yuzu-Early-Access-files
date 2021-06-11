// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/param_package.h"
#include "input_common/main.h"

namespace InputCommon::Polling {
class DevicePoller;
enum class DeviceType;
} // namespace InputCommon::Polling

namespace InputCommon::SDL {

class State {
public:
    using Pollers = std::vector<std::unique_ptr<Polling::DevicePoller>>;

    /// Unregisters SDL device factories and shut them down.
    virtual ~State() = default;

    virtual Pollers GetPollers(Polling::DeviceType) {
        return {};
    }

    virtual std::vector<Common::ParamPackage> GetInputDevices() {
        return {};
    }

    virtual ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage&) {
        return {};
    }
    virtual AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage&) {
        return {};
    }
    virtual MotionMapping GetMotionMappingForDevice(const Common::ParamPackage&) {
        return {};
    }
};

class NullState : public State {
public:
};

std::unique_ptr<State> Init();

} // namespace InputCommon::SDL
