// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
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
    explicit Controller_DebugPad(Core::HID::HIDCore& hid_core_);
    ~Controller_DebugPad() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

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
        s64 sampling_number;
        DebugPadAttribute attribute;
        Core::HID::DebugPadButton pad_state;
        Core::HID::AnalogStickState r_stick;
        Core::HID::AnalogStickState l_stick;
    };
    static_assert(sizeof(DebugPadState) == 0x20, "DebugPadState is an invalid state");

    // This is nn::hid::detail::DebugPadLifo
    Lifo<DebugPadState, hid_entry_count> debug_pad_lifo{};
    static_assert(sizeof(debug_pad_lifo) == 0x2C8, "debug_pad_lifo is an invalid size");
    DebugPadState next_state{};

    Core::HID::EmulatedController* controller;
};
} // namespace Service::HID
