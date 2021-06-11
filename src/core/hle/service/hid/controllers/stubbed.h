// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_Stubbed final : public ControllerBase {
public:
    explicit Controller_Stubbed(Core::System& system_);
    ~Controller_Stubbed() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

    void SetCommonHeaderOffset(std::size_t off);

private:
    bool smart_update{};
    std::size_t common_offset{};
};
} // namespace Service::HID
