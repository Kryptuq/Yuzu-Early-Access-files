// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <thread>

#include <QDialog>

#include "common/common_types.h"

enum class HIDButton : u8;

class InputInterpreter;

namespace Core {
class System;
}

namespace Ui {
class OverlayDialog;
}

/**
 * An OverlayDialog is an interactive dialog that accepts controller input (while a game is running)
 * This dialog attempts to replicate the look and feel of the Nintendo Switch's overlay dialogs and
 * provide some extra features such as embedding HTML/Rich Text content in a QTextBrowser.
 * The OverlayDialog provides 2 modes: one to embed regular text into a QLabel and another to embed
 * HTML/Rich Text content into a QTextBrowser.
 */
class OverlayDialog final : public QDialog {
    Q_OBJECT

public:
    explicit OverlayDialog(QWidget* parent, Core::System& system, const QString& title_text,
                           const QString& body_text, const QString& left_button_text,
                           const QString& right_button_text,
                           Qt::Alignment alignment = Qt::AlignCenter, bool use_rich_text_ = false);
    ~OverlayDialog() override;

private:
    /**
     * Initializes a text dialog with a QLabel storing text.
     * Only use this for short text as the dialog buttons would be squashed with longer text.
     *
     * @param title_text Title text to be displayed
     * @param body_text Main text to be displayed
     * @param left_button_text Left button text. If empty, the button is hidden and disabled
     * @param right_button_text Right button text. If empty, the button is hidden and disabled
     * @param alignment Main text alignment
     */
    void InitializeRegularTextDialog(const QString& title_text, const QString& body_text,
                                     const QString& left_button_text,
                                     const QString& right_button_text, Qt::Alignment alignment);

    /**
     * Initializes a text dialog with a QTextBrowser storing text.
     * This is ideal for longer text or rich text content. A scrollbar is shown for longer text.
     *
     * @param title_text Title text to be displayed
     * @param body_text Main text to be displayed
     * @param left_button_text Left button text. If empty, the button is hidden and disabled
     * @param right_button_text Right button text. If empty, the button is hidden and disabled
     * @param alignment Main text alignment
     */
    void InitializeRichTextDialog(const QString& title_text, const QString& body_text,
                                  const QString& left_button_text, const QString& right_button_text,
                                  Qt::Alignment alignment);

    /// Moves and resizes the dialog to be fully overlayed on top of the parent window.
    void MoveAndResizeWindow();

    /**
     * Handles button presses and converts them into keyboard input.
     *
     * @tparam HIDButton The list of buttons that can be converted into keyboard input.
     */
    template <HIDButton... T>
    void HandleButtonPressedOnce();

    /**
     * Translates a button press to focus or click either the left or right buttons.
     *
     * @param button The button press to process.
     */
    void TranslateButtonPress(HIDButton button);

    void StartInputThread();
    void StopInputThread();

    /// The thread where input is being polled and processed.
    void InputThread();

    std::unique_ptr<Ui::OverlayDialog> ui;

    bool use_rich_text;

    std::unique_ptr<InputInterpreter> input_interpreter;

    std::thread input_thread;

    std::atomic<bool> input_thread_running{};
};
