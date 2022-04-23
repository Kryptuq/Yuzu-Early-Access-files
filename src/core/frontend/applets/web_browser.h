// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "core/hle/service/am/applets/applet_web_browser_types.h"

namespace Core::Frontend {

class WebBrowserApplet {
public:
    virtual ~WebBrowserApplet();

    virtual void OpenLocalWebPage(
        const std::string& local_url, std::function<void()> extract_romfs_callback,
        std::function<void(Service::AM::Applets::WebExitReason, std::string)> callback) const = 0;

    virtual void OpenExternalWebPage(
        const std::string& external_url,
        std::function<void(Service::AM::Applets::WebExitReason, std::string)> callback) const = 0;
};

class DefaultWebBrowserApplet final : public WebBrowserApplet {
public:
    ~DefaultWebBrowserApplet() override;

    void OpenLocalWebPage(const std::string& local_url,
                          std::function<void()> extract_romfs_callback,
                          std::function<void(Service::AM::Applets::WebExitReason, std::string)>
                              callback) const override;

    void OpenExternalWebPage(const std::string& external_url,
                             std::function<void(Service::AM::Applets::WebExitReason, std::string)>
                                 callback) const override;
};

} // namespace Core::Frontend
