// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/frontend/input.h"
#include "input_common/mouse/mouse_input.h"

namespace InputCommon {

/**
 * A button device factory representing a mouse. It receives mouse events and forward them
 * to all button devices it created.
 */
class MouseButtonFactory final : public Input::Factory<Input::ButtonDevice> {
public:
    explicit MouseButtonFactory(std::shared_ptr<MouseInput::Mouse> mouse_input_);

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
    std::shared_ptr<MouseInput::Mouse> mouse_input;
    bool polling = false;
};

/// An analog device factory that creates analog devices from mouse
class MouseAnalogFactory final : public Input::Factory<Input::AnalogDevice> {
public:
    explicit MouseAnalogFactory(std::shared_ptr<MouseInput::Mouse> mouse_input_);

    std::unique_ptr<Input::AnalogDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetNextInput() const;

    /// For device input configuration/polling
    void BeginConfiguration();
    void EndConfiguration();

    bool IsPolling() const {
        return polling;
    }

private:
    std::shared_ptr<MouseInput::Mouse> mouse_input;
    bool polling = false;
};

/// A motion device factory that creates motion devices from mouse
class MouseMotionFactory final : public Input::Factory<Input::MotionDevice> {
public:
    explicit MouseMotionFactory(std::shared_ptr<MouseInput::Mouse> mouse_input_);

    std::unique_ptr<Input::MotionDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetNextInput() const;

    /// For device input configuration/polling
    void BeginConfiguration();
    void EndConfiguration();

    bool IsPolling() const {
        return polling;
    }

private:
    std::shared_ptr<MouseInput::Mouse> mouse_input;
    bool polling = false;
};

/// An touch device factory that creates touch devices from mouse
class MouseTouchFactory final : public Input::Factory<Input::TouchDevice> {
public:
    explicit MouseTouchFactory(std::shared_ptr<MouseInput::Mouse> mouse_input_);

    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetNextInput() const;

    /// For device input configuration/polling
    void BeginConfiguration();
    void EndConfiguration();

    bool IsPolling() const {
        return polling;
    }

private:
    std::shared_ptr<MouseInput::Mouse> mouse_input;
    bool polling = false;
};

} // namespace InputCommon
