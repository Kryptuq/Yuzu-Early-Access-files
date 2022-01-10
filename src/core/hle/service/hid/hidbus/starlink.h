// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/hle/service/hid/hidbus/hidbus_base.h"

namespace Core::HID {
class EmulatedController;
} // namespace Core::HID

namespace Service::HID {

class Starlink final : public HidbusBase {
public:
    explicit Starlink(Core::HID::HIDCore& hid_core_,
                      KernelHelpers::ServiceContext& service_context_);
    ~Starlink() override;

    void OnInit() override;

    void OnRelease() override;

    // Updates ringcon transfer memory
    void OnUpdate() override;

    // Returns the device ID of the joycon
    u8 GetDeviceId() const override;

    // Assigns a command from data
    bool SetCommand(const std::vector<u8>& data) override;

    // Returns a reply from a command
    std::vector<u8> GetReply() const override;
};

} // namespace Service::HID
