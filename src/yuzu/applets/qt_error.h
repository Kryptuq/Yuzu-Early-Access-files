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

    void ShowError(Result error, FinishedCallback finished) const override;
    void ShowErrorWithTimestamp(Result error, std::chrono::seconds time,
                                FinishedCallback finished) const override;
    void ShowCustomErrorText(Result error, std::string dialog_text, std::string fullscreen_text,
                             FinishedCallback finished) const override;

signals:
    void MainWindowDisplayError(QString error_code, QString error_text) const;

private:
    void MainWindowFinishedError();

    mutable FinishedCallback callback;
};
