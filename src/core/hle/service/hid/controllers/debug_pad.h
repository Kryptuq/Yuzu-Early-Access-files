// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/settings.h"
#include "common/swap.h"
#include "core/frontend/input.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_DebugPad final : public ControllerBase {
public:
    explicit Controller_DebugPad(Core::System& system_);
    ~Controller_DebugPad() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

private:
    struct AnalogStick {
        s32_le x;
        s32_le y;
    };
    static_assert(sizeof(AnalogStick) == 0x8);

    struct PadState {
        union {
            u32_le raw{};
            BitField<0, 1, u32> a;
            BitField<1, 1, u32> b;
            BitField<2, 1, u32> x;
            BitField<3, 1, u32> y;
            BitField<4, 1, u32> l;
            BitField<5, 1, u32> r;
            BitField<6, 1, u32> zl;
            BitField<7, 1, u32> zr;
            BitField<8, 1, u32> plus;
            BitField<9, 1, u32> minus;
            BitField<10, 1, u32> d_left;
            BitField<11, 1, u32> d_up;
            BitField<12, 1, u32> d_right;
            BitField<13, 1, u32> d_down;
        };
    };
    static_assert(sizeof(PadState) == 0x4, "PadState is an invalid size");

    struct Attributes {
        union {
            u32_le raw{};
            BitField<0, 1, u32> connected;
        };
    };
    static_assert(sizeof(Attributes) == 0x4, "Attributes is an invalid size");

    struct PadStates {
        s64_le sampling_number;
        s64_le sampling_number2;
        Attributes attribute;
        PadState pad_state;
        AnalogStick r_stick;
        AnalogStick l_stick;
    };
    static_assert(sizeof(PadStates) == 0x28, "PadStates is an invalid state");

    struct SharedMemory {
        CommonHeader header;
        std::array<PadStates, 17> pad_states;
        INSERT_PADDING_BYTES(0x138);
    };
    static_assert(sizeof(SharedMemory) == 0x400, "SharedMemory is an invalid size");
    SharedMemory shared_memory{};

    std::array<std::unique_ptr<Input::ButtonDevice>, Settings::NativeButton::NUM_BUTTONS_HID>
        buttons;
    std::array<std::unique_ptr<Input::AnalogDevice>, Settings::NativeAnalog::NUM_STICKS_HID>
        analogs;
};
} // namespace Service::HID
