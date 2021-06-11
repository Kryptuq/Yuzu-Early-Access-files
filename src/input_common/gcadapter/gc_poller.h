// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/frontend/input.h"
#include "input_common/gcadapter/gc_adapter.h"

namespace InputCommon {

/**
 * A button device factory representing a gcpad. It receives gcpad events and forward them
 * to all button devices it created.
 */
class GCButtonFactory final : public Input::Factory<Input::ButtonDevice> {
public:
    explicit GCButtonFactory(std::shared_ptr<GCAdapter::Adapter> adapter_);

    /**
     * Creates a button device from a button press
     * @param params contains parameters for creating the device:
     *     - "code": the code of the key to bind with the button
     */
    std::unique_ptr<Input::ButtonDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetNextInput() const;

    /// For device input configuration/polling
    void BeginConfiguration();
    void EndConfiguration();

    bool IsPolling() const {
        return polling;
    }

private:
    std::shared_ptr<GCAdapter::Adapter> adapter;
    bool polling = false;
};

/// An analog device factory that creates analog devices from GC Adapter
class GCAnalogFactory final : public Input::Factory<Input::AnalogDevice> {
public:
    explicit GCAnalogFactory(std::shared_ptr<GCAdapter::Adapter> adapter_);

    std::unique_ptr<Input::AnalogDevice> Create(const Common::ParamPackage& params) override;
    Common::ParamPackage GetNextInput();

    /// For device input configuration/polling
    void BeginConfiguration();
    void EndConfiguration();

    bool IsPolling() const {
        return polling;
    }

private:
    std::shared_ptr<GCAdapter::Adapter> adapter;
    int analog_x_axis = -1;
    int analog_y_axis = -1;
    int controller_number = -1;
    bool polling = false;
};

/// A vibration device factory creates vibration devices from GC Adapter
class GCVibrationFactory final : public Input::Factory<Input::VibrationDevice> {
public:
    explicit GCVibrationFactory(std::shared_ptr<GCAdapter::Adapter> adapter_);

    std::unique_ptr<Input::VibrationDevice> Create(const Common::ParamPackage& params) override;

private:
    std::shared_ptr<GCAdapter::Adapter> adapter;
};

} // namespace InputCommon
