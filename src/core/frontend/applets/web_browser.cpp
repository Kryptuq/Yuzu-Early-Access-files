// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/frontend/applets/web_browser.h"

namespace Core::Frontend {

WebBrowserApplet::~WebBrowserApplet() = default;

DefaultWebBrowserApplet::~DefaultWebBrowserApplet() = default;

void DefaultWebBrowserApplet::Close() const {}

void DefaultWebBrowserApplet::OpenLocalWebPage(const std::string& local_url,
                                               ExtractROMFSCallback extract_romfs_callback,
                                               OpenWebPageCallback callback) const {
    LOG_WARNING(Service_AM, "(STUBBED) called, backend requested to open local web page at {}",
                local_url);

    callback(Service::AM::Applets::WebExitReason::WindowClosed, "http://localhost/");
}

void DefaultWebBrowserApplet::OpenExternalWebPage(const std::string& external_url,
                                                  OpenWebPageCallback callback) const {
    LOG_WARNING(Service_AM, "(STUBBED) called, backend requested to open external web page at {}",
                external_url);

    callback(Service::AM::Applets::WebExitReason::WindowClosed, "http://localhost/");
}

} // namespace Core::Frontend
