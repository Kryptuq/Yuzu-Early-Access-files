// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/point.h"
#include "common/uuid.h"

namespace Core::HID {

// This is nn::hid::NpadIdType
enum class NpadIdType : u8 {
    Player1 = 0x0,
    Player2 = 0x1,
    Player3 = 0x2,
    Player4 = 0x3,
    Player5 = 0x4,
    Player6 = 0x5,
    Player7 = 0x6,
    Player8 = 0x7,
    Other = 0x10,
    Handheld = 0x20,

    Invalid = 0xFF,
};

/// Converts a NpadIdType to an array index.
constexpr size_t NpadIdTypeToIndex(NpadIdType npad_id_type) {
    switch (npad_id_type) {
    case NpadIdType::Player1:
        return 0;
    case NpadIdType::Player2:
        return 1;
    case NpadIdType::Player3:
        return 2;
    case NpadIdType::Player4:
        return 3;
    case NpadIdType::Player5:
        return 4;
    case NpadIdType::Player6:
        return 5;
    case NpadIdType::Player7:
        return 6;
    case NpadIdType::Player8:
        return 7;
    case NpadIdType::Handheld:
        return 8;
    case NpadIdType::Other:
        return 9;
    default:
        return 0;
    }
}

/// Converts an array index to a NpadIdType
constexpr NpadIdType IndexToNpadIdType(size_t index) {
    switch (index) {
    case 0:
        return NpadIdType::Player1;
    case 1:
        return NpadIdType::Player2;
    case 2:
        return NpadIdType::Player3;
    case 3:
        return NpadIdType::Player4;
    case 4:
        return NpadIdType::Player5;
    case 5:
        return NpadIdType::Player6;
    case 6:
        return NpadIdType::Player7;
    case 7:
        return NpadIdType::Player8;
    case 8:
        return NpadIdType::Handheld;
    case 9:
        return NpadIdType::Other;
    default:
        return NpadIdType::Invalid;
    }
}

// This is nn::hid::NpadType
enum class NpadType : u8 {
    None = 0,
    ProController = 3,
    Handheld = 4,
    JoyconDual = 5,
    JoyconLeft = 6,
    JoyconRight = 7,
    GameCube = 8,
    Pokeball = 9,
    MaxNpadType = 10,
};

// This is nn::hid::NpadStyleTag
struct NpadStyleTag {
    union {
        u32 raw{};

        BitField<0, 1, u32> fullkey;
        BitField<1, 1, u32> handheld;
        BitField<2, 1, u32> joycon_dual;
        BitField<3, 1, u32> joycon_left;
        BitField<4, 1, u32> joycon_right;
        BitField<5, 1, u32> gamecube;
        BitField<6, 1, u32> palma;
        BitField<7, 1, u32> lark;
        BitField<8, 1, u32> handheld_lark;
        BitField<9, 1, u32> lucia;
        BitField<10, 1, u32> lagoon;
        BitField<11, 1, u32> lager;
        BitField<29, 1, u32> system_ext;
        BitField<30, 1, u32> system;
    };
};
static_assert(sizeof(NpadStyleTag) == 4, "NpadStyleTag is an invalid size");

// This is nn::hid::TouchAttribute
struct TouchAttribute {
    union {
        u32 raw{};
        BitField<0, 1, u32> start_touch;
        BitField<1, 1, u32> end_touch;
    };
};
static_assert(sizeof(TouchAttribute) == 0x4, "TouchAttribute is an invalid size");

// This is nn::hid::TouchState
struct TouchState {
    u64 delta_time;
    TouchAttribute attribute;
    u32 finger;
    Common::Point<u32> position;
    u32 diameter_x;
    u32 diameter_y;
    u32 rotation_angle;
};
static_assert(sizeof(TouchState) == 0x28, "Touchstate is an invalid size");

// This is nn::hid::NpadControllerColor
struct NpadControllerColor {
    u32 body;
    u32 button;
};
static_assert(sizeof(NpadControllerColor) == 8, "NpadControllerColor is an invalid size");

// This is nn::hid::AnalogStickState
struct AnalogStickState {
    s32 x;
    s32 y;
};
static_assert(sizeof(AnalogStickState) == 8, "AnalogStickState is an invalid size");

// This is nn::hid::server::NpadGcTriggerState
struct NpadGcTriggerState {
    s64 sampling_number{};
    s32 left{};
    s32 right{};
};
static_assert(sizeof(NpadGcTriggerState) == 0x10, "NpadGcTriggerState is an invalid size");

// This is nn::hid::system::NpadBatteryLevel
using BatteryLevel = u32;
static_assert(sizeof(BatteryLevel) == 0x4, "BatteryLevel is an invalid size");

// This is nn::hid::system::NpadPowerInfo
struct NpadPowerInfo {
    bool is_powered;
    bool is_charging;
    INSERT_PADDING_BYTES(0x6);
    BatteryLevel battery_level;
};
static_assert(sizeof(NpadPowerInfo) == 0xC, "NpadPowerInfo is an invalid size");

struct LedPattern {
    explicit LedPattern(u64 light1, u64 light2, u64 light3, u64 light4) {
        position1.Assign(light1);
        position2.Assign(light2);
        position3.Assign(light3);
        position4.Assign(light4);
    }
    union {
        u64 raw{};
        BitField<0, 1, u64> position1;
        BitField<1, 1, u64> position2;
        BitField<2, 1, u64> position3;
        BitField<3, 1, u64> position4;
    };
};

// This is nn::hid::NpadButton
enum class NpadButton : u64 {
    None = 0,
    A = 1U << 0,
    B = 1U << 1,
    X = 1U << 2,
    Y = 1U << 3,
    StickL = 1U << 4,
    StickR = 1U << 5,
    L = 1U << 6,
    R = 1U << 7,
    ZL = 1U << 8,
    ZR = 1U << 9,
    Plus = 1U << 10,
    Minus = 1U << 11,

    Left = 1U << 12,
    Up = 1U << 13,
    Right = 1U << 14,
    Down = 1U << 15,

    StickLLeft = 1U << 16,
    StickLUp = 1U << 17,
    StickLRight = 1U << 18,
    StickLDown = 1U << 19,

    StickRLeft = 1U << 20,
    StickRUp = 1U << 21,
    StickRRight = 1U << 22,
    StickRDown = 1U << 23,

    LeftSL = 1U << 24,
    LeftSR = 1U << 25,

    RightSL = 1U << 26,
    RightSR = 1U << 27,

    Palma = 1U << 28,
    Verification = 1U << 29,
    HandheldLeftB = 1U << 30,
    LagonCLeft = 1U << 31,
    LagonCUp = 1ULL << 32,
    LagonCRight = 1ULL << 33,
    LagonCDown = 1ULL << 34,
};
DECLARE_ENUM_FLAG_OPERATORS(NpadButton);

struct NpadButtonState {
    union {
        NpadButton raw{};

        // Buttons
        BitField<0, 1, u64> a;
        BitField<1, 1, u64> b;
        BitField<2, 1, u64> x;
        BitField<3, 1, u64> y;
        BitField<4, 1, u64> stick_l;
        BitField<5, 1, u64> stick_r;
        BitField<6, 1, u64> l;
        BitField<7, 1, u64> r;
        BitField<8, 1, u64> zl;
        BitField<9, 1, u64> zr;
        BitField<10, 1, u64> plus;
        BitField<11, 1, u64> minus;

        // D-Pad
        BitField<12, 1, u64> left;
        BitField<13, 1, u64> up;
        BitField<14, 1, u64> right;
        BitField<15, 1, u64> down;

        // Left JoyStick
        BitField<16, 1, u64> stick_l_left;
        BitField<17, 1, u64> stick_l_up;
        BitField<18, 1, u64> stick_l_right;
        BitField<19, 1, u64> stick_l_down;

        // Right JoyStick
        BitField<20, 1, u64> stick_r_left;
        BitField<21, 1, u64> stick_r_up;
        BitField<22, 1, u64> stick_r_right;
        BitField<23, 1, u64> stick_r_down;

        BitField<24, 1, u64> left_sl;
        BitField<25, 1, u64> left_sr;

        BitField<26, 1, u64> right_sl;
        BitField<27, 1, u64> right_sr;

        BitField<28, 1, u64> palma;
        BitField<29, 1, u64> verification;
        BitField<30, 1, u64> handheld_left_b;
        BitField<31, 1, u64> lagon_c_left;
        BitField<32, 1, u64> lagon_c_up;
        BitField<33, 1, u64> lagon_c_right;
        BitField<34, 1, u64> lagon_c_down;
    };
};
static_assert(sizeof(NpadButtonState) == 0x8, "NpadButtonState has incorrect size.");

// This is nn::hid::DebugPadButton
struct DebugPadButton {
    union {
        u32 raw{};
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
static_assert(sizeof(DebugPadButton) == 0x4, "DebugPadButton is an invalid size");

// This is nn::hid::VibrationDeviceType
enum class VibrationDeviceType : u32 {
    Unknown = 0,
    LinearResonantActuator = 1,
    GcErm = 2,
};

// This is nn::hid::VibrationDevicePosition
enum class VibrationDevicePosition : u32 {
    None = 0,
    Left = 1,
    Right = 2,
};

// This is nn::hid::VibrationValue
struct VibrationValue {
    f32 low_amplitude;
    f32 low_frequency;
    f32 high_amplitude;
    f32 high_frequency;
};
static_assert(sizeof(VibrationValue) == 0x10, "VibrationValue has incorrect size.");

// This is nn::hid::VibrationGcErmCommand
enum class VibrationGcErmCommand : u64 {
    Stop = 0,
    Start = 1,
    StopHard = 2,
};

// This is nn::hid::VibrationDeviceInfo
struct VibrationDeviceInfo {
    VibrationDeviceType type{};
    VibrationDevicePosition position{};
};
static_assert(sizeof(VibrationDeviceInfo) == 0x8, "VibrationDeviceInfo has incorrect size.");

// This is nn::hid::KeyboardModifier
struct KeyboardModifier {
    union {
        u32 raw{};
        BitField<0, 1, u32> control;
        BitField<1, 1, u32> shift;
        BitField<2, 1, u32> left_alt;
        BitField<3, 1, u32> right_alt;
        BitField<4, 1, u32> gui;
        BitField<8, 1, u32> caps_lock;
        BitField<9, 1, u32> scroll_lock;
        BitField<10, 1, u32> num_lock;
        BitField<11, 1, u32> katakana;
        BitField<12, 1, u32> hiragana;
    };
};
static_assert(sizeof(KeyboardModifier) == 0x4, "KeyboardModifier is an invalid size");

// This is nn::hid::KeyboardKey
struct KeyboardKey {
    // This should be a 256 bit flag
    std::array<u8, 32> key;
};
static_assert(sizeof(KeyboardKey) == 0x20, "KeyboardKey is an invalid size");

// This is nn::hid::MouseButton
struct MouseButton {
    union {
        u32_le raw{};
        BitField<0, 1, u32> left;
        BitField<1, 1, u32> right;
        BitField<2, 1, u32> middle;
        BitField<3, 1, u32> forward;
        BitField<4, 1, u32> back;
    };
};
static_assert(sizeof(MouseButton) == 0x4, "MouseButton is an invalid size");

// This is nn::hid::MouseAttribute
struct MouseAttribute {
    union {
        u32 raw{};
        BitField<0, 1, u32> transferable;
        BitField<1, 1, u32> is_connected;
    };
};
static_assert(sizeof(MouseAttribute) == 0x4, "MouseAttribute is an invalid size");

// This is nn::hid::detail::MouseState
struct MouseState {
    s64 sampling_number;
    s32 x;
    s32 y;
    s32 delta_x;
    s32 delta_y;
    s32 delta_wheel_x;
    s32 delta_wheel_y;
    MouseButton button;
    MouseAttribute attribute;
};
static_assert(sizeof(MouseState) == 0x28, "MouseState is an invalid size");
} // namespace Core::HID
