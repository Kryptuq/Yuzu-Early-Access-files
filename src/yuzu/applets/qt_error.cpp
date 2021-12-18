// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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

void QtErrorDisplay::ShowError(ResultCode error, std::function<void()> finished) const {
    callback = std::move(finished);
    emit MainWindowDisplayError(
        tr("Error Code: %1-%2 (0x%3)")
            .arg(static_cast<u32>(error.module.Value()) + 2000, 4, 10, QChar::fromLatin1('0'))
            .arg(error.description, 4, 10, QChar::fromLatin1('0'))
            .arg(error.raw, 8, 16, QChar::fromLatin1('0')),
        tr("An error has occurred.\nPlease try again or contact the developer of the software."));
}

void QtErrorDisplay::ShowErrorWithTimestamp(ResultCode error, std::chrono::seconds time,
                                            std::function<void()> finished) const {
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

void QtErrorDisplay::ShowCustomErrorText(ResultCode error, std::string dialog_text,
                                         std::string fullscreen_text,
                                         std::function<void()> finished) const {
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
