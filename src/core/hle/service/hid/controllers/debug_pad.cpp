// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

Controller_DebugPad::Controller_DebugPad(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_)
    : ControllerBase{hid_core_} {
    static_assert(SHARED_MEMORY_OFFSET + sizeof(DebugPadSharedMemory) < shared_memory_size,
                  "DebugPadSharedMemory is bigger than the shared memory");
    shared_memory = std::construct_at(
        reinterpret_cast<DebugPadSharedMemory*>(raw_shared_memory_ + SHARED_MEMORY_OFFSET));
    controller = hid_core.GetEmulatedController(Core::HID::NpadIdType::Other);
}

Controller_DebugPad::~Controller_DebugPad() = default;

void Controller_DebugPad::OnInit() {}

void Controller_DebugPad::OnRelease() {}

void Controller_DebugPad::OnUpdate(const Core::Timing::CoreTiming& core_timing) {
    if (!IsControllerActivated()) {
        shared_memory->debug_pad_lifo.buffer_count = 0;
        shared_memory->debug_pad_lifo.buffer_tail = 0;
        return;
    }

    const auto& last_entry = shared_memory->debug_pad_lifo.ReadCurrentEntry().state;
    next_state.sampling_number = last_entry.sampling_number + 1;

    if (Settings::values.debug_pad_enabled) {
        next_state.attribute.connected.Assign(1);

        const auto& button_state = controller->GetDebugPadButtons();
        const auto& stick_state = controller->GetSticks();

        next_state.pad_state = button_state;
        next_state.l_stick = stick_state.left;
        next_state.r_stick = stick_state.right;
    }

    shared_memory->debug_pad_lifo.WriteNextEntry(next_state);
}

} // namespace Service::HID
