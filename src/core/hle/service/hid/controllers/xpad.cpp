// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "core/core_timing.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/controllers/xpad.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x3C00;

Controller_XPad::Controller_XPad(Core::HID::HIDCore& hid_core_) : ControllerBase{hid_core_} {}
Controller_XPad::~Controller_XPad() = default;

void Controller_XPad::OnInit() {}

void Controller_XPad::OnRelease() {}

void Controller_XPad::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                               std::size_t size) {
    if (!IsControllerActivated()) {
        basic_xpad_lifo.buffer_count = 0;
        basic_xpad_lifo.buffer_tail = 0;
        std::memcpy(data + SHARED_MEMORY_OFFSET, &basic_xpad_lifo, sizeof(basic_xpad_lifo));
        return;
    }

    const auto& last_entry = basic_xpad_lifo.ReadCurrentEntry().state;
    next_state.sampling_number = last_entry.sampling_number + 1;
    // TODO(ogniK): Update xpad states

    basic_xpad_lifo.WriteNextEntry(next_state);
    std::memcpy(data + SHARED_MEMORY_OFFSET, &basic_xpad_lifo, sizeof(basic_xpad_lifo));
}

} // namespace Service::HID
