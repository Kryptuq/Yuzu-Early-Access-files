// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef YUZU_USE_QT_WEB_ENGINE

#include <QObject>
#include <QWebEngineUrlRequestInterceptor>

class UrlRequestInterceptor : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT

public:
    explicit UrlRequestInterceptor(QObject* p = nullptr);
    ~UrlRequestInterceptor() override;

    void interceptRequest(QWebEngineUrlRequestInfo& info) override;

    QUrl GetRequestedURL() const;

signals:
    void FrameChanged();

private:
    QUrl requested_url;
};

#endif
