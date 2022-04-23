// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

#include "core/frontend/applets/error.h"

class GMainWindow;

class QtErrorDisplay final : public QObject, public Core::Frontend::ErrorApplet {
    Q_OBJECT

public:
    explicit QtErrorDisplay(GMainWindow& parent);
    ~QtErrorDisplay() override;

    void ShowError(ResultCode error, std::function<void()> finished) const override;
    void ShowErrorWithTimestamp(ResultCode error, std::chrono::seconds time,
                                std::function<void()> finished) const override;
    void ShowCustomErrorText(ResultCode error, std::string dialog_text, std::string fullscreen_text,
                             std::function<void()> finished) const override;

signals:
    void MainWindowDisplayError(QString error_code, QString error_text) const;

private:
    void MainWindowFinishedError();

    mutable std::function<void()> callback;
};
