// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
