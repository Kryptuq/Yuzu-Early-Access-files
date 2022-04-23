// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "common/uuid.h"

namespace Service::AM::Applets {

constexpr std::size_t MAX_OK_TEXT_LENGTH = 8;
constexpr std::size_t MAX_HEADER_TEXT_LENGTH = 64;
constexpr std::size_t MAX_SUB_TEXT_LENGTH = 128;
constexpr std::size_t MAX_GUIDE_TEXT_LENGTH = 256;
constexpr std::size_t STRING_BUFFER_SIZE = 0x7D4;

enum class SwkbdAppletVersion : u32_le {
    Version5 = 0x5,          // 1.0.0
    Version65542 = 0x10006,  // 2.0.0 - 2.3.0
    Version196615 = 0x30007, // 3.0.0 - 3.0.2
    Version262152 = 0x40008, // 4.0.0 - 4.1.0
    Version327689 = 0x50009, // 5.0.0 - 5.1.0
    Version393227 = 0x6000B, // 6.0.0 - 7.0.1
    Version524301 = 0x8000D, // 8.0.0+
};

enum class SwkbdType : u32 {
    Normal,
    NumberPad,
    Qwerty,
    Unknown3,
    Latin,
    SimplifiedChinese,
    TraditionalChinese,
    Korean,
};

enum class SwkbdInitialCursorPosition : u32 {
    Start,
    End,
};

enum class SwkbdPasswordMode : u32 {
    Disabled,
    Enabled,
};

enum class SwkbdTextDrawType : u32 {
    Line,
    Box,
    DownloadCode,
};

enum class SwkbdResult : u32 {
    Ok,
    Cancel,
};

enum class SwkbdTextCheckResult : u32 {
    Success,
    Failure,
    Confirm,
    Silent,
};

enum class SwkbdState : u32 {
    NotInitialized = 0x0,
    InitializedIsHidden = 0x1,
    InitializedIsAppearing = 0x2,
    InitializedIsShown = 0x3,
    InitializedIsDisappearing = 0x4,
};

enum class SwkbdRequestCommand : u32 {
    Finalize = 0x4,
    SetUserWordInfo = 0x6,
    SetCustomizeDic = 0x7,
    Calc = 0xA,
    SetCustomizedDictionaries = 0xB,
    UnsetCustomizedDictionaries = 0xC,
    SetChangedStringV2Flag = 0xD,
    SetMovedCursorV2Flag = 0xE,
};

enum class SwkbdReplyType : u32 {
    FinishedInitialize = 0x0,
    Default = 0x1,
    ChangedString = 0x2,
    MovedCursor = 0x3,
    MovedTab = 0x4,
    DecidedEnter = 0x5,
    DecidedCancel = 0x6,
    ChangedStringUtf8 = 0x7,
    MovedCursorUtf8 = 0x8,
    DecidedEnterUtf8 = 0x9,
    UnsetCustomizeDic = 0xA,
    ReleasedUserWordInfo = 0xB,
    UnsetCustomizedDictionaries = 0xC,
    ChangedStringV2 = 0xD,
    MovedCursorV2 = 0xE,
    ChangedStringUtf8V2 = 0xF,
    MovedCursorUtf8V2 = 0x10,
};

struct SwkbdKeyDisableFlags {
    union {
        u32 raw{};

        BitField<1, 1, u32> space;
        BitField<2, 1, u32> at;
        BitField<3, 1, u32> percent;
        BitField<4, 1, u32> slash;
        BitField<5, 1, u32> backslash;
        BitField<6, 1, u32> numbers;
        BitField<7, 1, u32> download_code;
        BitField<8, 1, u32> username;
    };
};
static_assert(sizeof(SwkbdKeyDisableFlags) == 0x4, "SwkbdKeyDisableFlags has incorrect size.");

struct SwkbdConfigCommon {
    SwkbdType type{};
    std::array<char16_t, MAX_OK_TEXT_LENGTH + 1> ok_text{};
    char16_t left_optional_symbol_key{};
    char16_t right_optional_symbol_key{};
    bool use_prediction{};
    INSERT_PADDING_BYTES(1);
    SwkbdKeyDisableFlags key_disable_flags{};
    SwkbdInitialCursorPosition initial_cursor_position{};
    std::array<char16_t, MAX_HEADER_TEXT_LENGTH + 1> header_text{};
    std::array<char16_t, MAX_SUB_TEXT_LENGTH + 1> sub_text{};
    std::array<char16_t, MAX_GUIDE_TEXT_LENGTH + 1> guide_text{};
    u32 max_text_length{};
    u32 min_text_length{};
    SwkbdPasswordMode password_mode{};
    SwkbdTextDrawType text_draw_type{};
    bool enable_return_button{};
    bool use_utf8{};
    bool use_blur_background{};
    INSERT_PADDING_BYTES(1);
    u32 initial_string_offset{};
    u32 initial_string_length{};
    u32 user_dictionary_offset{};
    u32 user_dictionary_entries{};
    bool use_text_check{};
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(SwkbdConfigCommon) == 0x3D4, "SwkbdConfigCommon has incorrect size.");

#pragma pack(push, 4)
// SwkbdAppletVersion 0x5, 0x10006
struct SwkbdConfigOld {
    INSERT_PADDING_WORDS(1);
    VAddr text_check_callback{};
};
static_assert(sizeof(SwkbdConfigOld) == 0x3E0 - sizeof(SwkbdConfigCommon),
              "SwkbdConfigOld has incorrect size.");

// SwkbdAppletVersion 0x30007, 0x40008, 0x50009
struct SwkbdConfigOld2 {
    INSERT_PADDING_WORDS(1);
    VAddr text_check_callback{};
    std::array<u32, 8> text_grouping{};
};
static_assert(sizeof(SwkbdConfigOld2) == 0x400 - sizeof(SwkbdConfigCommon),
              "SwkbdConfigOld2 has incorrect size.");

// SwkbdAppletVersion 0x6000B, 0x8000D
struct SwkbdConfigNew {
    std::array<u32, 8> text_grouping{};
    std::array<u64, 24> customized_dictionary_set_entries{};
    u8 total_customized_dictionary_set_entries{};
    bool disable_cancel_button{};
    INSERT_PADDING_BYTES(18);
};
static_assert(sizeof(SwkbdConfigNew) == 0x4C8 - sizeof(SwkbdConfigCommon),
              "SwkbdConfigNew has incorrect size.");
#pragma pack(pop)

struct SwkbdTextCheck {
    SwkbdTextCheckResult text_check_result{};
    std::array<char16_t, STRING_BUFFER_SIZE / 2> text_check_message{};
};
static_assert(sizeof(SwkbdTextCheck) == 0x7D8, "SwkbdTextCheck has incorrect size.");

struct SwkbdCalcArgFlags {
    union {
        u64 raw{};

        BitField<0, 1, u64> set_initialize_arg;
        BitField<1, 1, u64> set_volume;
        BitField<2, 1, u64> appear;
        BitField<3, 1, u64> set_input_text;
        BitField<4, 1, u64> set_cursor_position;
        BitField<5, 1, u64> set_utf8_mode;
        BitField<6, 1, u64> unset_customize_dic;
        BitField<7, 1, u64> disappear;
        BitField<8, 1, u64> unknown;
        BitField<9, 1, u64> set_key_top_translate_scale;
        BitField<10, 1, u64> unset_user_word_info;
        BitField<11, 1, u64> set_disable_hardware_keyboard;
    };
};
static_assert(sizeof(SwkbdCalcArgFlags) == 0x8, "SwkbdCalcArgFlags has incorrect size.");

struct SwkbdInitializeArg {
    u32 unknown{};
    bool library_applet_mode_flag{};
    bool is_above_hos_500{};
    INSERT_PADDING_BYTES(2);
};
static_assert(sizeof(SwkbdInitializeArg) == 0x8, "SwkbdInitializeArg has incorrect size.");

struct SwkbdAppearArgOld {
    SwkbdType type{};
    std::array<char16_t, MAX_OK_TEXT_LENGTH + 1> ok_text{};
    char16_t left_optional_symbol_key{};
    char16_t right_optional_symbol_key{};
    bool use_prediction{};
    bool disable_cancel_button{};
    SwkbdKeyDisableFlags key_disable_flags{};
    u32 max_text_length{};
    u32 min_text_length{};
    bool enable_return_button{};
    INSERT_PADDING_BYTES(3);
    u32 flags{};
    bool is_use_save_data{};
    INSERT_PADDING_BYTES(7);
    Common::UUID user_id{};
};
static_assert(sizeof(SwkbdAppearArgOld) == 0x48, "SwkbdAppearArg has incorrect size.");

struct SwkbdAppearArgNew {
    SwkbdType type{};
    std::array<char16_t, MAX_OK_TEXT_LENGTH + 1> ok_text{};
    char16_t left_optional_symbol_key{};
    char16_t right_optional_symbol_key{};
    bool use_prediction{};
    bool disable_cancel_button{};
    SwkbdKeyDisableFlags key_disable_flags{};
    u32 max_text_length{};
    u32 min_text_length{};
    bool enable_return_button{};
    INSERT_PADDING_BYTES(3);
    u32 flags{};
    bool is_use_save_data{};
    INSERT_PADDING_BYTES(7);
    Common::UUID user_id{};
    u64 start_sampling_number{};
    INSERT_PADDING_WORDS(8);
};
static_assert(sizeof(SwkbdAppearArgNew) == 0x70, "SwkbdAppearArg has incorrect size.");

struct SwkbdCalcArgCommon {
    u32 unknown{};
    u16 calc_arg_size{};
    INSERT_PADDING_BYTES(2);
    SwkbdCalcArgFlags flags{};
    SwkbdInitializeArg initialize_arg{};
};
static_assert(sizeof(SwkbdCalcArgCommon) == 0x18, "SwkbdCalcArgCommon has incorrect size.");

struct SwkbdCalcArgOld {
    f32 volume{};
    s32 cursor_position{};
    SwkbdAppearArgOld appear_arg{};
    std::array<char16_t, 0x1FA> input_text{};
    bool utf8_mode{};
    INSERT_PADDING_BYTES(1);
    bool enable_backspace_button{};
    INSERT_PADDING_BYTES(3);
    bool key_top_as_floating{};
    bool footer_scalable{};
    bool alpha_enabled_in_input_mode{};
    u8 input_mode_fade_type{};
    bool disable_touch{};
    bool disable_hardware_keyboard{};
    INSERT_PADDING_BYTES(8);
    f32 key_top_scale_x{};
    f32 key_top_scale_y{};
    f32 key_top_translate_x{};
    f32 key_top_translate_y{};
    f32 key_top_bg_alpha{};
    f32 footer_bg_alpha{};
    f32 balloon_scale{};
    INSERT_PADDING_WORDS(4);
    u8 se_group{};
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(SwkbdCalcArgOld) == 0x4A0 - sizeof(SwkbdCalcArgCommon),
              "SwkbdCalcArgOld has incorrect size.");

struct SwkbdCalcArgNew {
    SwkbdAppearArgNew appear_arg{};
    f32 volume{};
    s32 cursor_position{};
    std::array<char16_t, 0x1FA> input_text{};
    bool utf8_mode{};
    INSERT_PADDING_BYTES(1);
    bool enable_backspace_button{};
    INSERT_PADDING_BYTES(3);
    bool key_top_as_floating{};
    bool footer_scalable{};
    bool alpha_enabled_in_input_mode{};
    u8 input_mode_fade_type{};
    bool disable_touch{};
    bool disable_hardware_keyboard{};
    INSERT_PADDING_BYTES(8);
    f32 key_top_scale_x{};
    f32 key_top_scale_y{};
    f32 key_top_translate_x{};
    f32 key_top_translate_y{};
    f32 key_top_bg_alpha{};
    f32 footer_bg_alpha{};
    f32 balloon_scale{};
    INSERT_PADDING_WORDS(4);
    u8 se_group{};
    INSERT_PADDING_BYTES(3);
    INSERT_PADDING_WORDS(8);
};
static_assert(sizeof(SwkbdCalcArgNew) == 0x4E8 - sizeof(SwkbdCalcArgCommon),
              "SwkbdCalcArgNew has incorrect size.");

struct SwkbdChangedStringArg {
    u32 text_length{};
    s32 dictionary_start_cursor_position{};
    s32 dictionary_end_cursor_position{};
    s32 cursor_position{};
};
static_assert(sizeof(SwkbdChangedStringArg) == 0x10, "SwkbdChangedStringArg has incorrect size.");

struct SwkbdMovedCursorArg {
    u32 text_length{};
    s32 cursor_position{};
};
static_assert(sizeof(SwkbdMovedCursorArg) == 0x8, "SwkbdMovedCursorArg has incorrect size.");

struct SwkbdMovedTabArg {
    u32 text_length{};
    s32 cursor_position{};
};
static_assert(sizeof(SwkbdMovedTabArg) == 0x8, "SwkbdMovedTabArg has incorrect size.");

struct SwkbdDecidedEnterArg {
    u32 text_length{};
};
static_assert(sizeof(SwkbdDecidedEnterArg) == 0x4, "SwkbdDecidedEnterArg has incorrect size.");

} // namespace Service::AM::Applets
