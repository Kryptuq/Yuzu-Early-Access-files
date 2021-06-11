// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "core/core_timing.h"
#include "core/hle/service/hid/controllers/xpad.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3C00;

Controller_XPad::Controller_XPad(Core::System& system_) : ControllerBase{system_} {}
Controller_XPad::~Controller_XPad() = default;

void Controller_XPad::OnInit() {}

void Controller_XPad::OnRelease() {}

void Controller_XPad::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                               std::size_t size) {
    for (auto& xpad_entry : shared_memory.shared_memory_entries) {
        xpad_entry.header.timestamp = core_timing.GetCPUTicks();
        xpad_entry.header.total_entry_count = 17;

        if (!IsControllerActivated()) {
            xpad_entry.header.entry_count = 0;
            xpad_entry.header.last_entry_index = 0;
            return;
        }
        xpad_entry.header.entry_count = 16;

        const auto& last_entry = xpad_entry.pad_states[xpad_entry.header.last_entry_index];
        xpad_entry.header.last_entry_index = (xpad_entry.header.last_entry_index + 1) % 17;
        auto& cur_entry = xpad_entry.pad_states[xpad_entry.header.last_entry_index];

        cur_entry.sampling_number = last_entry.sampling_number + 1;
        cur_entry.sampling_number2 = cur_entry.sampling_number;
    }
    // TODO(ogniK): Update xpad states

    std::memcpy(data + SHARED_MEMORY_OFFSET, &shared_memory, sizeof(SharedMemory));
}

void Controller_XPad::OnLoadInputDevices() {}
} // namespace Service::HID
