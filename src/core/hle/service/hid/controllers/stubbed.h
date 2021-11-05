// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_Stubbed final : public ControllerBase {
public:
    explicit Controller_Stubbed(Core::HID::HIDCore& hid_core_);
    ~Controller_Stubbed() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    void SetCommonHeaderOffset(std::size_t off);

private:
    struct CommonHeader {
        s64 timestamp;
        s64 total_entry_count;
        s64 last_entry_index;
        s64 entry_count;
    };
    static_assert(sizeof(CommonHeader) == 0x20, "CommonHeader is an invalid size");

    bool smart_update{};
    std::size_t common_offset{};
};
} // namespace Service::HID
