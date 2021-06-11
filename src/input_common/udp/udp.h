// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/frontend/input.h"
#include "input_common/udp/client.h"

namespace InputCommon {

/// A motion device factory that creates motion devices from udp clients
class UDPMotionFactory final : public Input::Factory<Input::MotionDevice> {
public:
    explicit UDPMotionFactory(std::shared_ptr<CemuhookUDP::Client> client_);

    std::unique_ptr<Input::MotionDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetNextInput();

    /// For device input configuration/polling
    void BeginConfiguration();
    void EndConfiguration();

    bool IsPolling() const {
        return polling;
    }

private:
    std::shared_ptr<CemuhookUDP::Client> client;
    bool polling = false;
};

/// A touch device factory that creates touch devices from udp clients
class UDPTouchFactory final : public Input::Factory<Input::TouchDevice> {
public:
    explicit UDPTouchFactory(std::shared_ptr<CemuhookUDP::Client> client_);

    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage& params) override;

    Common::ParamPackage GetNextInput();

    /// For device input configuration/polling
    void BeginConfiguration();
    void EndConfiguration();

    bool IsPolling() const {
        return polling;
    }

private:
    std::shared_ptr<CemuhookUDP::Client> client;
    bool polling = false;
};

} // namespace InputCommon
