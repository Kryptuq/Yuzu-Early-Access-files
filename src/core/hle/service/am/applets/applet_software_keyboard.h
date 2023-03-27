// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/result.h"
#include "core/hle/service/am/applets/applet_software_keyboard_types.h"
#include "core/hle/service/am/applets/applets.h"

namespace Core {
class System;
}

namespace Core::Frontend {
struct KeyboardInitializeParameters;
struct InlineAppearParameters;
} // namespace Core::Frontend

namespace Service::AM::Applets {

class SoftwareKeyboard final : public Applet {
public:
    explicit SoftwareKeyboard(Core::System& system_, LibraryAppletMode applet_mode_,
                              Core::Frontend::SoftwareKeyboardApplet& frontend_);
    ~SoftwareKeyboard() override;

    void Initialize() override;

    bool TransactionComplete() const override;
    Result GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;
    Result RequestExit() override;

    /**
     * Submits the input text to the application.
     * If text checking is enabled, the application will verify the input text.
     * If use_utf8 is enabled, the input text will be converted to UTF-8 prior to being submitted.
     * This should only be used by the normal software keyboard.
     *
     * @param result SwkbdResult enum
     * @param submitted_text UTF-16 encoded string
     * @param confirmed Whether the text has been confirmed after TextCheckResult::Confirm
     */
    void SubmitTextNormal(SwkbdResult result, std::u16string submitted_text, bool confirmed);

    /**
     * Submits the input text to the application.
     * If utf8_mode is enabled, the input text will be converted to UTF-8 prior to being submitted.
     * This should only be used by the inline software keyboard.
     *
     * @param reply_type SwkbdReplyType enum
     * @param submitted_text UTF-16 encoded string
     * @param cursor_position The current position of the text cursor
     */
    void SubmitTextInline(SwkbdReplyType reply_type, std::u16string submitted_text,
                          s32 cursor_position);

private:
    /// Initializes the normal software keyboard.
    void InitializeForeground();

    /// Initializes the inline software keyboard.
    void InitializeBackground(LibraryAppletMode library_applet_mode);

    /// Processes the text check sent by the application.
    void ProcessTextCheck();

    /// Processes the inline software keyboard request command sent by the application.
    void ProcessInlineKeyboardRequest();

    /// Submits the input text and exits the applet.
    void SubmitNormalOutputAndExit(SwkbdResult result, std::u16string submitted_text);

    /// Submits the input text for text checking.
    void SubmitForTextCheck(std::u16string submitted_text);

    /// Sends a reply to the application after processing a request command.
    void SendReply(SwkbdReplyType reply_type);

    /// Changes the inline keyboard state.
    void ChangeState(SwkbdState state);

    /**
     * Signals the frontend to initialize the normal software keyboard with common parameters.
     * Note that this does not cause the keyboard to appear.
     * Use the ShowNormalKeyboard() functions to cause the keyboard to appear.
     */
    void InitializeFrontendNormalKeyboard();

    /**
     * Signals the frontend to initialize the inline software keyboard with common parameters.
     * Note that this does not cause the keyboard to appear.
     * Use the ShowInlineKeyboard() to cause the keyboard to appear.
     */
    void InitializeFrontendInlineKeyboard(
        Core::Frontend::KeyboardInitializeParameters initialize_parameters);

    void InitializeFrontendInlineKeyboardOld();
    void InitializeFrontendInlineKeyboardNew();

    /// Signals the frontend to show the normal software keyboard.
    void ShowNormalKeyboard();

    /// Signals the frontend to show the text check dialog.
    void ShowTextCheckDialog(SwkbdTextCheckResult text_check_result,
                             std::u16string text_check_message);

    /// Signals the frontend to show the inline software keyboard.
    void ShowInlineKeyboard(Core::Frontend::InlineAppearParameters appear_parameters);

    void ShowInlineKeyboardOld();
    void ShowInlineKeyboardNew();

    /// Signals the frontend to hide the inline software keyboard.
    void HideInlineKeyboard();

    /// Signals the frontend that the current inline keyboard text has changed.
    void InlineTextChanged();

    /// Signals both the frontend and application that the software keyboard is exiting.
    void ExitKeyboard();

    // Inline Software Keyboard Requests

    void RequestFinalize(const std::vector<u8>& request_data);
    void RequestSetUserWordInfo(const std::vector<u8>& request_data);
    void RequestSetCustomizeDic(const std::vector<u8>& request_data);
    void RequestCalc(const std::vector<u8>& request_data);
    void RequestCalcOld();
    void RequestCalcNew();
    void RequestSetCustomizedDictionaries(const std::vector<u8>& request_data);
    void RequestUnsetCustomizedDictionaries(const std::vector<u8>& request_data);
    void RequestSetChangedStringV2Flag(const std::vector<u8>& request_data);
    void RequestSetMovedCursorV2Flag(const std::vector<u8>& request_data);

    // Inline Software Keyboard Replies

    void ReplyFinishedInitialize();
    void ReplyDefault();
    void ReplyChangedString();
    void ReplyMovedCursor();
    void ReplyMovedTab();
    void ReplyDecidedEnter();
    void ReplyDecidedCancel();
    void ReplyChangedStringUtf8();
    void ReplyMovedCursorUtf8();
    void ReplyDecidedEnterUtf8();
    void ReplyUnsetCustomizeDic();
    void ReplyReleasedUserWordInfo();
    void ReplyUnsetCustomizedDictionaries();
    void ReplyChangedStringV2();
    void ReplyMovedCursorV2();
    void ReplyChangedStringUtf8V2();
    void ReplyMovedCursorUtf8V2();

    Core::Frontend::SoftwareKeyboardApplet& frontend;
    Core::System& system;

    SwkbdAppletVersion swkbd_applet_version;

    SwkbdConfigCommon swkbd_config_common;
    SwkbdConfigOld swkbd_config_old;
    SwkbdConfigOld2 swkbd_config_old2;
    SwkbdConfigNew swkbd_config_new;
    std::u16string initial_text;

    SwkbdState swkbd_state{SwkbdState::NotInitialized};
    SwkbdInitializeArg swkbd_initialize_arg;
    SwkbdCalcArgCommon swkbd_calc_arg_common;
    SwkbdCalcArgOld swkbd_calc_arg_old;
    SwkbdCalcArgNew swkbd_calc_arg_new;
    bool use_changed_string_v2{false};
    bool use_moved_cursor_v2{false};
    bool inline_use_utf8{false};
    s32 current_cursor_position{};

    std::u16string current_text;

    bool is_background{false};

    bool complete{false};
    Result status{ResultSuccess};
};

} // namespace Service::AM::Applets
