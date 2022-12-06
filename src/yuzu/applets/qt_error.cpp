// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDateTime>
#include "yuzu/applets/qt_error.h"
#include "yuzu/main.h"

QtErrorDisplay::QtErrorDisplay(GMainWindow& parent) {
    connect(this, &QtErrorDisplay::MainWindowDisplayError, &parent,
            &GMainWindow::ErrorDisplayDisplayError, Qt::QueuedConnection);
    connect(&parent, &GMainWindow::ErrorDisplayFinished, this,
            &QtErrorDisplay::MainWindowFinishedError, Qt::DirectConnection);
}

QtErrorDisplay::~QtErrorDisplay() = default;

void QtErrorDisplay::ShowError(Result error, FinishedCallback finished) const {
    callback = std::move(finished);
    emit MainWindowDisplayError(
        tr("Error Code: %1-%2 (0x%3)")
            .arg(static_cast<u32>(error.module.Value()) + 2000, 4, 10, QChar::fromLatin1('0'))
            .arg(error.description, 4, 10, QChar::fromLatin1('0'))
            .arg(error.raw, 8, 16, QChar::fromLatin1('0')),
        tr("An error has occurred.\nPlease try again or contact the developer of the software."));
}

void QtErrorDisplay::ShowErrorWithTimestamp(Result error, std::chrono::seconds time,
                                            FinishedCallback finished) const {
    callback = std::move(finished);

    const QDateTime date_time = QDateTime::fromSecsSinceEpoch(time.count());
    emit MainWindowDisplayError(
        tr("Error Code: %1-%2 (0x%3)")
            .arg(static_cast<u32>(error.module.Value()) + 2000, 4, 10, QChar::fromLatin1('0'))
            .arg(error.description, 4, 10, QChar::fromLatin1('0'))
            .arg(error.raw, 8, 16, QChar::fromLatin1('0')),
        tr("An error occurred on %1 at %2.\nPlease try again or contact the developer of the "
           "software.")
            .arg(date_time.toString(QStringLiteral("dddd, MMMM d, yyyy")))
            .arg(date_time.toString(QStringLiteral("h:mm:ss A"))));
}

void QtErrorDisplay::ShowCustomErrorText(Result error, std::string dialog_text,
                                         std::string fullscreen_text,
                                         FinishedCallback finished) const {
    callback = std::move(finished);
    emit MainWindowDisplayError(
        tr("Error Code: %1-%2 (0x%3)")
            .arg(static_cast<u32>(error.module.Value()) + 2000, 4, 10, QChar::fromLatin1('0'))
            .arg(error.description, 4, 10, QChar::fromLatin1('0'))
            .arg(error.raw, 8, 16, QChar::fromLatin1('0')),
        tr("An error has occurred.\n\n%1\n\n%2")
            .arg(QString::fromStdString(dialog_text))
            .arg(QString::fromStdString(fullscreen_text)));
}

void QtErrorDisplay::MainWindowFinishedError() {
    callback();
}
