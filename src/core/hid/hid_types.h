// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/point.h"
#include "common/uuid.h"

namespace Core::HID {

enum class DeviceIndex : u8 {
    Left = 0,
    Right = 1,
    None = 2,
    MaxDeviceIndex = 3,
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

    All = 0xFFFFFFFFFFFFFFFFULL,
};
DECLARE_ENUM_FLAG_OPERATORS(NpadButton);

enum class KeyboardKeyIndex : u32 {
    A = 4,
    B = 5,
    C = 6,
    D = 7,
    E = 8,
    F = 9,
    G = 10,
    H = 11,
    I = 12,
    J = 13,
    K = 14,
    L = 15,
    M = 16,
    N = 17,
    O = 18,
    P = 19,
    Q = 20,
    R = 21,
    S = 22,
    T = 23,
    U = 24,
    V = 25,
    W = 26,
    X = 27,
    Y = 28,
    Z = 29,
    D1 = 30,
    D2 = 31,
    D3 = 32,
    D4 = 33,
    D5 = 34,
    D6 = 35,
    D7 = 36,
    D8 = 37,
    D9 = 38,
    D0 = 39,
    Return = 40,
    Escape = 41,
    Backspace = 42,
    Tab = 43,
    Space = 44,
    Minus = 45,
    Plus = 46,
    OpenBracket = 47,
    CloseBracket = 48,
    Pipe = 49,
    Tilde = 50,
    Semicolon = 51,
    Quote = 52,
    Backquote = 53,
    Comma = 54,
    Period = 55,
    Slash = 56,
    CapsLock = 57,
    F1 = 58,
    F2 = 59,
    F3 = 60,
    F4 = 61,
    F5 = 62,
    F6 = 63,
    F7 = 64,
    F8 = 65,
    F9 = 66,
    F10 = 67,
    F11 = 68,
    F12 = 69,
    PrintScreen = 70,
    ScrollLock = 71,
    Pause = 72,
    Insert = 73,
    Home = 74,
    PageUp = 75,
    Delete = 76,
    End = 77,
    PageDown = 78,
    RightArrow = 79,
    LeftArrow = 80,
    DownArrow = 81,
    UpArrow = 82,
    NumLock = 83,
    NumPadDivide = 84,
    NumPadMultiply = 85,
    NumPadSubtract = 86,
    NumPadAdd = 87,
    NumPadEnter = 88,
    NumPad1 = 89,
    NumPad2 = 90,
    NumPad3 = 91,
    NumPad4 = 92,
    NumPad5 = 93,
    NumPad6 = 94,
    NumPad7 = 95,
    NumPad8 = 96,
    NumPad9 = 97,
    NumPad0 = 98,
    NumPadDot = 99,
    Backslash = 100,
    Application = 101,
    Power = 102,
    NumPadEquals = 103,
    F13 = 104,
    F14 = 105,
    F15 = 106,
    F16 = 107,
    F17 = 108,
    F18 = 109,
    F19 = 110,
    F20 = 111,
    F21 = 112,
    F22 = 113,
    F23 = 114,
    F24 = 115,
    NumPadComma = 133,
    Ro = 135,
    KatakanaHiragana = 136,
    Yen = 137,
    Henkan = 138,
    Muhenkan = 139,
    NumPadCommaPc98 = 140,
    HangulEnglish = 144,
    Hanja = 145,
    Katakana = 146,
    Hiragana = 147,
    ZenkakuHankaku = 148,
    LeftControl = 224,
    LeftShift = 225,
    LeftAlt = 226,
    LeftGui = 227,
    RightControl = 228,
    RightShift = 229,
    RightAlt = 230,
    RightGui = 231,
};

// This is nn::hid::NpadIdType
enum class NpadIdType : u32 {
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

    Invalid = 0xFFFFFFFF,
};

// This is nn::hid::NpadStyleIndex
enum class NpadStyleIndex : u8 {
    None = 0,
    ProController = 3,
    Handheld = 4,
    HandheldNES = 4,
    JoyconDual = 5,
    JoyconLeft = 6,
    JoyconRight = 7,
    GameCube = 8,
    Pokeball = 9,
    NES = 10,
    SNES = 12,
    N64 = 13,
    SegaGenesis = 14,
    SystemExt = 32,
    System = 33,
    MaxNpadType = 34,
};

// This is nn::hid::NpadStyleSet
enum class NpadStyleSet : u32 {
    None = 0,
    Fullkey = 1U << 0,
    Handheld = 1U << 1,
    JoyDual = 1U << 2,
    JoyLeft = 1U << 3,
    JoyRight = 1U << 4,
    Gc = 1U << 5,
    Palma = 1U << 6,
    Lark = 1U << 7,
    HandheldLark = 1U << 8,
    Lucia = 1U << 9,
    Lagoon = 1U << 10,
    Lager = 1U << 11,
    SystemExt = 1U << 29,
    System = 1U << 30,

    All = 0xFFFFFFFFU,
};
static_assert(sizeof(NpadStyleSet) == 4, "NpadStyleSet is an invalid size");

// This is nn::hid::VibrationDevicePosition
enum class VibrationDevicePosition : u32 {
    None = 0,
    Left = 1,
    Right = 2,
};

// This is nn::hid::VibrationDeviceType
enum class VibrationDeviceType : u32 {
    Unknown = 0,
    LinearResonantActuator = 1,
    GcErm = 2,
    N64 = 3,
};

// This is nn::hid::VibrationGcErmCommand
enum class VibrationGcErmCommand : u64 {
    Stop = 0,
    Start = 1,
    StopHard = 2,
};

// This is nn::hid::NpadStyleTag
struct NpadStyleTag {
    union {
        NpadStyleSet raw{};

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
    u64 delta_time{};
    TouchAttribute attribute{};
    u32 finger{};
    Common::Point<u32> position{};
    u32 diameter_x{};
    u32 diameter_y{};
    u32 rotation_angle{};
};
static_assert(sizeof(TouchState) == 0x28, "Touchstate is an invalid size");

// This is nn::hid::NpadControllerColor
struct NpadControllerColor {
    u32 body{};
    u32 button{};
};
static_assert(sizeof(NpadControllerColor) == 8, "NpadControllerColor is an invalid size");

// This is nn::hid::AnalogStickState
struct AnalogStickState {
    s32 x{};
    s32 y{};
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
using NpadBatteryLevel = u32;
static_assert(sizeof(NpadBatteryLevel) == 0x4, "NpadBatteryLevel is an invalid size");

// This is nn::hid::system::NpadPowerInfo
struct NpadPowerInfo {
    bool is_powered{};
    bool is_charging{};
    INSERT_PADDING_BYTES(0x6);
    NpadBatteryLevel battery_level{8};
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

struct HomeButtonState {
    union {
        u64 raw{};

        // Buttons
        BitField<0, 1, u64> home;
    };
};
static_assert(sizeof(HomeButtonState) == 0x8, "HomeButtonState has incorrect size.");

struct CaptureButtonState {
    union {
        u64 raw{};

        // Buttons
        BitField<0, 1, u64> capture;
    };
};
static_assert(sizeof(CaptureButtonState) == 0x8, "CaptureButtonState has incorrect size.");

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

// This is nn::hid::ConsoleSixAxisSensorHandle
struct ConsoleSixAxisSensorHandle {
    u8 unknown_1{};
    u8 unknown_2{};
    INSERT_PADDING_BYTES_NOINIT(2);
};
static_assert(sizeof(ConsoleSixAxisSensorHandle) == 4,
              "ConsoleSixAxisSensorHandle is an invalid size");

// This is nn::hid::SixAxisSensorHandle
struct SixAxisSensorHandle {
    NpadStyleIndex npad_type{NpadStyleIndex::None};
    u8 npad_id{};
    DeviceIndex device_index{DeviceIndex::None};
    INSERT_PADDING_BYTES_NOINIT(1);
};
static_assert(sizeof(SixAxisSensorHandle) == 4, "SixAxisSensorHandle is an invalid size");

// These parameters seem related to how much gyro/accelerometer is used
struct SixAxisSensorFusionParameters {
    f32 parameter1{0.03f}; // Range 0.0 to 1.0, default 0.03
    f32 parameter2{0.4f};  // Default 0.4
};
static_assert(sizeof(SixAxisSensorFusionParameters) == 8,
              "SixAxisSensorFusionParameters is an invalid size");

// This is nn::hid::server::SixAxisSensorProperties
struct SixAxisSensorProperties {
    union {
        u8 raw{};
        BitField<0, 1, u8> is_newly_assigned;
        BitField<1, 1, u8> is_firmware_update_available;
    };
};
static_assert(sizeof(SixAxisSensorProperties) == 1, "SixAxisSensorProperties is an invalid size");

// This is nn::hid::SixAxisSensorCalibrationParameter
struct SixAxisSensorCalibrationParameter {
    std::array<u8, 0x744> unknown_data{};
};
static_assert(sizeof(SixAxisSensorCalibrationParameter) == 0x744,
              "SixAxisSensorCalibrationParameter is an invalid size");

// This is nn::hid::SixAxisSensorIcInformation
struct SixAxisSensorIcInformation {
    f32 angular_rate{2000.0f}; // dps
    std::array<f32, 6> unknown_gyro_data1{
        -10.0f, -10.0f, -10.0f, 10.0f, 10.0f, 10.0f,
    }; // dps
    std::array<f32, 9> unknown_gyro_data2{
        0.95f, -0.003f, -0.003f, -0.003f, 0.95f, -0.003f, -0.003f, -0.003f, 0.95f,
    };
    std::array<f32, 9> unknown_gyro_data3{
        1.05f, 0.003f, 0.003f, 0.003f, 1.05f, 0.003f, 0.003f, 0.003f, 1.05f,
    };
    f32 acceleration_range{8.0f}; // g force
    std::array<f32, 6> unknown_accel_data1{
        -0.0612f, -0.0612f, -0.0612f, 0.0612f, 0.0612f, 0.0612f,
    }; // g force
    std::array<f32, 9> unknown_accel_data2{
        0.95f, -0.003f, -0.003f, -0.003f, 0.95f, -0.003f, -0.003f, -0.003f, 0.95f,
    };
    std::array<f32, 9> unknown_accel_data3{
        1.05f, 0.003f, 0.003f, 0.003f, 1.05f, 0.003f, 0.003f, 0.003f, 1.05f,
    };
};
static_assert(sizeof(SixAxisSensorIcInformation) == 0xC8,
              "SixAxisSensorIcInformation is an invalid size");

// This is nn::hid::VibrationDeviceHandle
struct VibrationDeviceHandle {
    NpadStyleIndex npad_type{NpadStyleIndex::None};
    u8 npad_id{};
    DeviceIndex device_index{DeviceIndex::None};
    INSERT_PADDING_BYTES_NOINIT(1);
};
static_assert(sizeof(VibrationDeviceHandle) == 4, "SixAxisSensorHandle is an invalid size");

// This is nn::hid::VibrationValue
struct VibrationValue {
    f32 low_amplitude{};
    f32 low_frequency{};
    f32 high_amplitude{};
    f32 high_frequency{};
};
static_assert(sizeof(VibrationValue) == 0x10, "VibrationValue has incorrect size.");

constexpr VibrationValue DEFAULT_VIBRATION_VALUE{
    .low_amplitude = 0.0f,
    .low_frequency = 160.0f,
    .high_amplitude = 0.0f,
    .high_frequency = 320.0f,
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

// This is nn::hid::KeyboardAttribute
struct KeyboardAttribute {
    union {
        u32 raw{};
        BitField<0, 1, u32> is_connected;
    };
};
static_assert(sizeof(KeyboardAttribute) == 0x4, "KeyboardAttribute is an invalid size");

// This is nn::hid::KeyboardKey
struct KeyboardKey {
    // This should be a 256 bit flag
    std::array<u8, 32> key{};
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
    s64 sampling_number{};
    s32 x{};
    s32 y{};
    s32 delta_x{};
    s32 delta_y{};
    // Axis Order in HW is switched for the wheel
    s32 delta_wheel_y{};
    s32 delta_wheel_x{};
    MouseButton button{};
    MouseAttribute attribute{};
};
static_assert(sizeof(MouseState) == 0x28, "MouseState is an invalid size");

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

} // namespace Core::HID
