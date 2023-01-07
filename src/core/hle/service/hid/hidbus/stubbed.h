// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/service/hid/hidbus/hidbus_base.h"

namespace Core::HID {
class EmulatedController;
} // namespace Core::HID

namespace Service::HID {

class HidbusStubbed final : public HidbusBase {
public:
    explicit HidbusStubbed(Core::HID::HIDCore& hid_core_,
                           KernelHelpers::ServiceContext& service_context_);
    ~HidbusStubbed() override;

    void OnInit() override;

    void OnRelease() override;

    // Updates ringcon transfer memory
    void OnUpdate() override;

    // Returns the device ID of the joycon
    u8 GetDeviceId() const override;

    // Assigns a command from data
    bool SetCommand(std::span<const u8> data) override;

    // Returns a reply from a command
    std::vector<u8> GetReply() const override;
};

} // namespace Service::HID
