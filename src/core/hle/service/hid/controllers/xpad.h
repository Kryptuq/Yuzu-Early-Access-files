// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hid/hid_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Service::HID {
class Controller_XPad final : public ControllerBase {
public:
    explicit Controller_XPad(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_);
    ~Controller_XPad() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

private:
    // This is nn::hid::BasicXpadAttributeSet
    struct BasicXpadAttributeSet {
        union {
            u32 raw{};
            BitField<0, 1, u32> is_connected;
            BitField<1, 1, u32> is_wired;
            BitField<2, 1, u32> is_left_connected;
            BitField<3, 1, u32> is_left_wired;
            BitField<4, 1, u32> is_right_connected;
            BitField<5, 1, u32> is_right_wired;
        };
    };
    static_assert(sizeof(BasicXpadAttributeSet) == 4, "BasicXpadAttributeSet is an invalid size");

    // This is nn::hid::BasicXpadButtonSet
    struct BasicXpadButtonSet {
        union {
            u32 raw{};
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
    static_assert(sizeof(BasicXpadButtonSet) == 4, "BasicXpadButtonSet is an invalid size");

    // This is nn::hid::detail::BasicXpadState
    struct BasicXpadState {
        s64 sampling_number{};
        BasicXpadAttributeSet attributes{};
        BasicXpadButtonSet pad_states{};
        Core::HID::AnalogStickState l_stick{};
        Core::HID::AnalogStickState r_stick{};
    };
    static_assert(sizeof(BasicXpadState) == 0x20, "BasicXpadState is an invalid size");

    struct XpadSharedMemory {
        // This is nn::hid::detail::BasicXpadLifo
        Lifo<BasicXpadState, hid_entry_count> basic_xpad_lifo{};
        static_assert(sizeof(basic_xpad_lifo) == 0x2C8, "basic_xpad_lifo is an invalid size");
        INSERT_PADDING_WORDS(0x4E);
    };
    static_assert(sizeof(XpadSharedMemory) == 0x400, "XpadSharedMemory is an invalid size");

    BasicXpadState next_state{};
    XpadSharedMemory* shared_memory = nullptr;
};
} // namespace Service::HID
