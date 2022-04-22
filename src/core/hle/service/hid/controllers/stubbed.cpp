// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "core/core_timing.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/stubbed.h"

namespace Service::HID {

Controller_Stubbed::Controller_Stubbed(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_)
    : ControllerBase{hid_core_} {
    raw_shared_memory = raw_shared_memory_;
}

Controller_Stubbed::~Controller_Stubbed() = default;

void Controller_Stubbed::OnInit() {}

void Controller_Stubbed::OnRelease() {}

void Controller_Stubbed::OnUpdate(const Core::Timing::CoreTiming& core_timing) {
    if (!smart_update) {
        return;
    }

    CommonHeader header{};
    header.timestamp = core_timing.GetCPUTicks();
    header.total_entry_count = 17;
    header.entry_count = 0;
    header.last_entry_index = 0;

    std::memcpy(raw_shared_memory + common_offset, &header, sizeof(CommonHeader));
}

void Controller_Stubbed::SetCommonHeaderOffset(std::size_t off) {
    common_offset = off;
    smart_update = true;
}

} // namespace Service::HID
