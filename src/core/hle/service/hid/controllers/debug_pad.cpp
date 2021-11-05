// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hid/hid_types.h"
#include "core/hle/service/hid/controllers/debug_pad.h"

namespace Service::HID {
constexpr std::size_t SHARED_MEMORY_OFFSET = 0x00000;

Controller_DebugPad::Controller_DebugPad(Core::HID::HIDCore& hid_core_)
    : ControllerBase{hid_core_} {
    controller = hid_core.GetEmulatedController(Core::HID::NpadIdType::Other);
}

Controller_DebugPad::~Controller_DebugPad() = default;

void Controller_DebugPad::OnInit() {}

void Controller_DebugPad::OnRelease() {}

void Controller_DebugPad::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                   std::size_t size) {
    if (!IsControllerActivated()) {
        debug_pad_lifo.buffer_count = 0;
        debug_pad_lifo.buffer_tail = 0;
        std::memcpy(data + SHARED_MEMORY_OFFSET, &debug_pad_lifo, sizeof(debug_pad_lifo));
        return;
    }

    const auto& last_entry = debug_pad_lifo.ReadCurrentEntry().state;
    next_state.sampling_number = last_entry.sampling_number + 1;

    if (Settings::values.debug_pad_enabled) {
        next_state.attribute.connected.Assign(1);

        const auto& button_state = controller->GetDebugPadButtons();
        const auto& stick_state = controller->GetSticks();

        next_state.pad_state = button_state;
        next_state.l_stick = stick_state.left;
        next_state.r_stick = stick_state.right;
    }

    debug_pad_lifo.WriteNextEntry(next_state);
    std::memcpy(data + SHARED_MEMORY_OFFSET, &debug_pad_lifo, sizeof(debug_pad_lifo));
}

} // namespace Service::HID
