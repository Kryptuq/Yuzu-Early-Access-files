// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_XPad final : public ControllerBase {
public:
    explicit Controller_XPad(Core::System& system_);
    ~Controller_XPad() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

private:
    struct Attributes {
        union {
            u32_le raw{};
            BitField<0, 1, u32> is_connected;
            BitField<1, 1, u32> is_wired;
            BitField<2, 1, u32> is_left_connected;
            BitField<3, 1, u32> is_left_wired;
            BitField<4, 1, u32> is_right_connected;
            BitField<5, 1, u32> is_right_wired;
        };
    };
    static_assert(sizeof(Attributes) == 4, "Attributes is an invalid size");

    struct Buttons {
        union {
            u32_le raw{};
            // Button states
            BitField<0, 1, u32> a;
            BitField<1, 1, u32> b;
            BitField<2, 1, u32> x;
            BitField<3, 1, u32> y;
            BitField<4, 1, u32> l_stick;
            BitField<5, 1, u32> r_stick;
            BitField<6, 1, u32> l;
            BitField<7, 1, u32> r;
            BitField<8, 1, u32> zl;
            BitField<9, 1, u32> zr;
            BitField<10, 1, u32> plus;
            BitField<11, 1, u32> minus;

            // D-Pad
            BitField<12, 1, u32> d_left;
            BitField<13, 1, u32> d_up;
            BitField<14, 1, u32> d_right;
            BitField<15, 1, u32> d_down;

            // Left JoyStick
            BitField<16, 1, u32> l_stick_left;
            BitField<17, 1, u32> l_stick_up;
            BitField<18, 1, u32> l_stick_right;
            BitField<19, 1, u32> l_stick_down;

            // Right JoyStick
            BitField<20, 1, u32> r_stick_left;
            BitField<21, 1, u32> r_stick_up;
            BitField<22, 1, u32> r_stick_right;
            BitField<23, 1, u32> r_stick_down;

            // Not always active?
            BitField<24, 1, u32> left_sl;
            BitField<25, 1, u32> left_sr;

            BitField<26, 1, u32> right_sl;
            BitField<27, 1, u32> right_sr;

            BitField<28, 1, u32> palma;
            BitField<30, 1, u32> handheld_left_b;
        };
    };
    static_assert(sizeof(Buttons) == 4, "Buttons is an invalid size");

    struct AnalogStick {
        s32_le x;
        s32_le y;
    };
    static_assert(sizeof(AnalogStick) == 0x8, "AnalogStick is an invalid size");

    struct XPadState {
        s64_le sampling_number;
        s64_le sampling_number2;
        Attributes attributes;
        Buttons pad_states;
        AnalogStick l_stick;
        AnalogStick r_stick;
    };
    static_assert(sizeof(XPadState) == 0x28, "XPadState is an invalid size");

    struct XPadEntry {
        CommonHeader header;
        std::array<XPadState, 17> pad_states{};
        INSERT_PADDING_BYTES(0x138);
    };
    static_assert(sizeof(XPadEntry) == 0x400, "XPadEntry is an invalid size");

    struct SharedMemory {
        std::array<XPadEntry, 4> shared_memory_entries{};
    };
    static_assert(sizeof(SharedMemory) == 0x1000, "SharedMemory is an invalid size");
    SharedMemory shared_memory{};
};
} // namespace Service::HID
