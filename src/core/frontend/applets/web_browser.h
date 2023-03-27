// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "core/frontend/applets/applet.h"
#include "core/hle/service/am/applets/applet_web_browser_types.h"

namespace Core::Frontend {

class WebBrowserApplet : public Applet {
public:
    using ExtractROMFSCallback = std::function<void()>;
    using OpenWebPageCallback =
        std::function<void(Service::AM::Applets::WebExitReason, std::string)>;

    virtual ~WebBrowserApplet();

    virtual void OpenLocalWebPage(const std::string& local_url,
                                  ExtractROMFSCallback extract_romfs_callback,
                                  OpenWebPageCallback callback) const = 0;

    virtual void OpenExternalWebPage(const std::string& external_url,
                                     OpenWebPageCallback callback) const = 0;
};

class DefaultWebBrowserApplet final : public WebBrowserApplet {
public:
    ~DefaultWebBrowserApplet() override;

    void Close() const override;

    void OpenLocalWebPage(const std::string& local_url, ExtractROMFSCallback extract_romfs_callback,
                          OpenWebPageCallback callback) const override;

    void OpenExternalWebPage(const std::string& external_url,
                             OpenWebPageCallback callback) const override;
};

} // namespace Core::Frontend
