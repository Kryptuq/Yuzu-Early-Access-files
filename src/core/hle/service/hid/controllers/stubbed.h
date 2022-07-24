// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_Stubbed final : public ControllerBase {
public:
    explicit Controller_Stubbed(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_);
    ~Controller_Stubbed() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

    void SetCommonHeaderOffset(std::size_t off);

private:
    struct CommonHeader {
        s64 timestamp{};
        s64 total_entry_count{};
        s64 last_entry_index{};
        s64 entry_count{};
    };
    static_assert(sizeof(CommonHeader) == 0x20, "CommonHeader is an invalid size");

    u8* raw_shared_memory = nullptr;
    bool smart_update{};
    std::size_t common_offset{};
};
} // namespace Service::HID
