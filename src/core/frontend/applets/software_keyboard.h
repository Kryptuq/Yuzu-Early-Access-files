// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <thread>

#include "common/common_types.h"

#include "core/hle/service/am/applets/applet_software_keyboard_types.h"

namespace Core::Frontend {

struct KeyboardInitializeParameters {
    std::u16string ok_text;
    std::u16string header_text;
    std::u16string sub_text;
    std::u16string guide_text;
    std::u16string initial_text;
    u32 max_text_length;
    u32 min_text_length;
    s32 initial_cursor_position;
    Service::AM::Applets::SwkbdType type;
    Service::AM::Applets::SwkbdPasswordMode password_mode;
    Service::AM::Applets::SwkbdTextDrawType text_draw_type;
    Service::AM::Applets::SwkbdKeyDisableFlags key_disable_flags;
    bool use_blur_background;
    bool enable_backspace_button;
    bool enable_return_button;
    bool disable_cancel_button;
};

struct InlineAppearParameters {
    u32 max_text_length;
    u32 min_text_length;
    f32 key_top_scale_x;
    f32 key_top_scale_y;
    f32 key_top_translate_x;
    f32 key_top_translate_y;
    Service::AM::Applets::SwkbdType type;
    Service::AM::Applets::SwkbdKeyDisableFlags key_disable_flags;
    bool key_top_as_floating;
    bool enable_backspace_button;
    bool enable_return_button;
    bool disable_cancel_button;
};

struct InlineTextParameters {
    std::u16string input_text;
    s32 cursor_position;
};

class SoftwareKeyboardApplet {
public:
    virtual ~SoftwareKeyboardApplet();

    virtual void InitializeKeyboard(
        bool is_inline, KeyboardInitializeParameters initialize_parameters,
        std::function<void(Service::AM::Applets::SwkbdResult, std::u16string)>
            submit_normal_callback_,
        std::function<void(Service::AM::Applets::SwkbdReplyType, std::u16string, s32)>
            submit_inline_callback_) = 0;

    virtual void ShowNormalKeyboard() const = 0;

    virtual void ShowTextCheckDialog(Service::AM::Applets::SwkbdTextCheckResult text_check_result,
                                     std::u16string text_check_message) const = 0;

    virtual void ShowInlineKeyboard(InlineAppearParameters appear_parameters) const = 0;

    virtual void HideInlineKeyboard() const = 0;

    virtual void InlineTextChanged(InlineTextParameters text_parameters) const = 0;

    virtual void ExitKeyboard() const = 0;
};

class DefaultSoftwareKeyboardApplet final : public SoftwareKeyboardApplet {
public:
    ~DefaultSoftwareKeyboardApplet() override;

    void InitializeKeyboard(
        bool is_inline, KeyboardInitializeParameters initialize_parameters,
        std::function<void(Service::AM::Applets::SwkbdResult, std::u16string)>
            submit_normal_callback_,
        std::function<void(Service::AM::Applets::SwkbdReplyType, std::u16string, s32)>
            submit_inline_callback_) override;

    void ShowNormalKeyboard() const override;

    void ShowTextCheckDialog(Service::AM::Applets::SwkbdTextCheckResult text_check_result,
                             std::u16string text_check_message) const override;

    void ShowInlineKeyboard(InlineAppearParameters appear_parameters) const override;

    void HideInlineKeyboard() const override;

    void InlineTextChanged(InlineTextParameters text_parameters) const override;

    void ExitKeyboard() const override;

private:
    void SubmitNormalText(std::u16string text) const;
    void SubmitInlineText(std::u16string_view text) const;

    KeyboardInitializeParameters parameters;

    mutable std::function<void(Service::AM::Applets::SwkbdResult, std::u16string)>
        submit_normal_callback;
    mutable std::function<void(Service::AM::Applets::SwkbdReplyType, std::u16string, s32)>
        submit_inline_callback;
};

} // namespace Core::Frontend
