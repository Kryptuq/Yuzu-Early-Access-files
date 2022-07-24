// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Core::HID {
class EmulatedController;
struct DebugPadButton;
struct AnalogStickState;
} // namespace Core::HID

namespace Service::HID {
class Controller_DebugPad final : public ControllerBase {
public:
    explicit Controller_DebugPad(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_);
    ~Controller_DebugPad() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

private:
    // This is nn::hid::DebugPadAttribute
    struct DebugPadAttribute {
        union {
            u32 raw{};
            BitField<0, 1, u32> connected;
        };
    };
    static_assert(sizeof(DebugPadAttribute) == 0x4, "DebugPadAttribute is an invalid size");

    // This is nn::hid::DebugPadState
    struct DebugPadState {
        s64 sampling_number{};
        DebugPadAttribute attribute{};
        Core::HID::DebugPadButton pad_state{};
        Core::HID::AnalogStickState r_stick{};
        Core::HID::AnalogStickState l_stick{};
    };
    static_assert(sizeof(DebugPadState) == 0x20, "DebugPadState is an invalid state");

    struct DebugPadSharedMemory {
        // This is nn::hid::detail::DebugPadLifo
        Lifo<DebugPadState, hid_entry_count> debug_pad_lifo{};
        static_assert(sizeof(debug_pad_lifo) == 0x2C8, "debug_pad_lifo is an invalid size");
        INSERT_PADDING_WORDS(0x4E);
    };
    static_assert(sizeof(DebugPadSharedMemory) == 0x400, "DebugPadSharedMemory is an invalid size");

    DebugPadState next_state{};
    DebugPadSharedMemory* shared_memory = nullptr;
    Core::HID::EmulatedController* controller = nullptr;
};
} // namespace Service::HID
