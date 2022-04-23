// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>

#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/frontend/applets/software_keyboard.h"

namespace Core::Frontend {

SoftwareKeyboardApplet::~SoftwareKeyboardApplet() = default;

DefaultSoftwareKeyboardApplet::~DefaultSoftwareKeyboardApplet() = default;

void DefaultSoftwareKeyboardApplet::InitializeKeyboard(
    bool is_inline, KeyboardInitializeParameters initialize_parameters,
    std::function<void(Service::AM::Applets::SwkbdResult, std::u16string, bool)>
        submit_normal_callback_,
    std::function<void(Service::AM::Applets::SwkbdReplyType, std::u16string, s32)>
        submit_inline_callback_) {
    if (is_inline) {
        LOG_WARNING(
            Service_AM,
            "(STUBBED) called, backend requested to initialize the inline software keyboard.");

        submit_inline_callback = std::move(submit_inline_callback_);
    } else {
        LOG_WARNING(
            Service_AM,
            "(STUBBED) called, backend requested to initialize the normal software keyboard.");

        submit_normal_callback = std::move(submit_normal_callback_);
    }

    parameters = std::move(initialize_parameters);

    LOG_INFO(Service_AM,
             "\nKeyboardInitializeParameters:"
             "\nok_text={}"
             "\nheader_text={}"
             "\nsub_text={}"
             "\nguide_text={}"
             "\ninitial_text={}"
             "\nmax_text_length={}"
             "\nmin_text_length={}"
             "\ninitial_cursor_position={}"
             "\ntype={}"
             "\npassword_mode={}"
             "\ntext_draw_type={}"
             "\nkey_disable_flags={}"
             "\nuse_blur_background={}"
             "\nenable_backspace_button={}"
             "\nenable_return_button={}"
             "\ndisable_cancel_button={}",
             Common::UTF16ToUTF8(parameters.ok_text), Common::UTF16ToUTF8(parameters.header_text),
             Common::UTF16ToUTF8(parameters.sub_text), Common::UTF16ToUTF8(parameters.guide_text),
             Common::UTF16ToUTF8(parameters.initial_text), parameters.max_text_length,
             parameters.min_text_length, parameters.initial_cursor_position, parameters.type,
             parameters.password_mode, parameters.text_draw_type, parameters.key_disable_flags.raw,
             parameters.use_blur_background, parameters.enable_backspace_button,
             parameters.enable_return_button, parameters.disable_cancel_button);
}

void DefaultSoftwareKeyboardApplet::ShowNormalKeyboard() const {
    LOG_WARNING(Service_AM,
                "(STUBBED) called, backend requested to show the normal software keyboard.");

    SubmitNormalText(u"yuzu");
}

void DefaultSoftwareKeyboardApplet::ShowTextCheckDialog(
    Service::AM::Applets::SwkbdTextCheckResult text_check_result,
    std::u16string text_check_message) const {
    LOG_WARNING(Service_AM, "(STUBBED) called, backend requested to show the text check dialog.");
}

void DefaultSoftwareKeyboardApplet::ShowInlineKeyboard(
    InlineAppearParameters appear_parameters) const {
    LOG_WARNING(Service_AM,
                "(STUBBED) called, backend requested to show the inline software keyboard.");

    LOG_INFO(Service_AM,
             "\nInlineAppearParameters:"
             "\nmax_text_length={}"
             "\nmin_text_length={}"
             "\nkey_top_scale_x={}"
             "\nkey_top_scale_y={}"
             "\nkey_top_translate_x={}"
             "\nkey_top_translate_y={}"
             "\ntype={}"
             "\nkey_disable_flags={}"
             "\nkey_top_as_floating={}"
             "\nenable_backspace_button={}"
             "\nenable_return_button={}"
             "\ndisable_cancel_button={}",
             appear_parameters.max_text_length, appear_parameters.min_text_length,
             appear_parameters.key_top_scale_x, appear_parameters.key_top_scale_y,
             appear_parameters.key_top_translate_x, appear_parameters.key_top_translate_y,
             appear_parameters.type, appear_parameters.key_disable_flags.raw,
             appear_parameters.key_top_as_floating, appear_parameters.enable_backspace_button,
             appear_parameters.enable_return_button, appear_parameters.disable_cancel_button);

    std::thread([this] { SubmitInlineText(u"yuzu"); }).detach();
}

void DefaultSoftwareKeyboardApplet::HideInlineKeyboard() const {
    LOG_WARNING(Service_AM,
                "(STUBBED) called, backend requested to hide the inline software keyboard.");
}

void DefaultSoftwareKeyboardApplet::InlineTextChanged(InlineTextParameters text_parameters) const {
    LOG_WARNING(Service_AM,
                "(STUBBED) called, backend requested to change the inline keyboard text.");

    LOG_INFO(Service_AM,
             "\nInlineTextParameters:"
             "\ninput_text={}"
             "\ncursor_position={}",
             Common::UTF16ToUTF8(text_parameters.input_text), text_parameters.cursor_position);

    submit_inline_callback(Service::AM::Applets::SwkbdReplyType::ChangedString,
                           text_parameters.input_text, text_parameters.cursor_position);
}

void DefaultSoftwareKeyboardApplet::ExitKeyboard() const {
    LOG_WARNING(Service_AM, "(STUBBED) called, backend requested to exit the software keyboard.");
}

void DefaultSoftwareKeyboardApplet::SubmitNormalText(std::u16string text) const {
    submit_normal_callback(Service::AM::Applets::SwkbdResult::Ok, text, true);
}

void DefaultSoftwareKeyboardApplet::SubmitInlineText(std::u16string_view text) const {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (std::size_t index = 0; index < text.size(); ++index) {
        submit_inline_callback(Service::AM::Applets::SwkbdReplyType::ChangedString,
                               std::u16string(text.data(), text.data() + index + 1),
                               static_cast<s32>(index) + 1);

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    submit_inline_callback(Service::AM::Applets::SwkbdReplyType::DecidedEnter, std::u16string(text),
                           static_cast<s32>(text.size()));
}

} // namespace Core::Frontend
