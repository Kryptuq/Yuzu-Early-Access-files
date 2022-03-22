// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>

#include <QObject>

#ifdef YUZU_USE_QT_WEB_ENGINE
#include <QWebEngineView>
#endif

#include "core/frontend/applets/web_browser.h"

class GMainWindow;
class InputInterpreter;
class UrlRequestInterceptor;

namespace Core {
class System;
}

namespace Core::HID {
enum class NpadButton : u64;
}

namespace InputCommon {
class InputSubsystem;
}

#ifdef YUZU_USE_QT_WEB_ENGINE

enum class UserAgent {
    WebApplet,
    ShopN,
    LoginApplet,
    ShareApplet,
    LobbyApplet,
    WifiWebAuthApplet,
};

class QWebEngineProfile;
class QWebEngineSettings;

class QtNXWebEngineView : public QWebEngineView {
    Q_OBJECT

public:
    explicit QtNXWebEngineView(QWidget* parent, Core::System& system,
                               InputCommon::InputSubsystem* input_subsystem_);
    ~QtNXWebEngineView() override;

    /**
     * Loads a HTML document that exists locally. Cannot be used to load external websites.
     *
     * @param main_url The url to the file.
     * @param additional_args Additional arguments appended to the main url.
     */
    void LoadLocalWebPage(const std::string& main_url, const std::string& additional_args);

    /**
     * Loads an external website. Cannot be used to load local urls.
     *
     * @param main_url The url to the website.
     * @param additional_args Additional arguments appended to the main url.
     */
    void LoadExternalWebPage(const std::string& main_url, const std::string& additional_args);

    /**
     * Sets the background color of the web page.
     *
     * @param color The color to set.
     */
    void SetBackgroundColor(QColor color);

    /**
     * Sets the user agent of the web browser.
     *
     * @param user_agent The user agent enum.
     */
    void SetUserAgent(UserAgent user_agent);

    [[nodiscard]] bool IsFinished() const;
    void SetFinished(bool finished_);

    [[nodiscard]] Service::AM::Applets::WebExitReason GetExitReason() const;
    void SetExitReason(Service::AM::Applets::WebExitReason exit_reason_);

    [[nodiscard]] const std::string& GetLastURL() const;
    void SetLastURL(std::string last_url_);

    /**
     * This gets the current URL that has been requested by the webpage.
     * This only applies to the main frame. Sub frames and other resources are ignored.
     *
     * @return Currently requested URL
     */
    [[nodiscard]] QString GetCurrentURL() const;

public slots:
    void hide();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    /**
     * Handles button presses to execute functions assigned in yuzu_key_callbacks.
     * yuzu_key_callbacks contains specialized functions for the buttons in the window footer
     * that can be overriden by games to achieve desired functionality.
     *
     * @tparam HIDButton The list of buttons contained in yuzu_key_callbacks
     */
    template <Core::HID::NpadButton... T>
    void HandleWindowFooterButtonPressedOnce();

    /**
     * Handles button presses and converts them into keyboard input.
     * This should only be used to convert D-Pad or Analog Stick input into arrow keys.
     *
     * @tparam HIDButton The list of buttons that can be converted into keyboard input.
     */
    template <Core::HID::NpadButton... T>
    void HandleWindowKeyButtonPressedOnce();

    /**
     * Handles button holds and converts them into keyboard input.
     * This should only be used to convert D-Pad or Analog Stick input into arrow keys.
     *
     * @tparam HIDButton The list of buttons that can be converted into keyboard input.
     */
    template <Core::HID::NpadButton... T>
    void HandleWindowKeyButtonHold();

    /**
     * Sends a key press event to QWebEngineView.
     *
     * @param key Qt key code.
     */
    void SendKeyPressEvent(int key);

    /**
     * Sends multiple key press events to QWebEngineView.
     *
     * @tparam int Qt key code.
     */
    template <int... T>
    void SendMultipleKeyPressEvents() {
        (SendKeyPressEvent(T), ...);
    }

    void StartInputThread();
    void StopInputThread();

    /// The thread where input is being polled and processed.
    void InputThread();

    /// Loads the extracted fonts using JavaScript.
    void LoadExtractedFonts();

    /// Brings focus to the first available link element.
    void FocusFirstLinkElement();

    InputCommon::InputSubsystem* input_subsystem;

    std::unique_ptr<UrlRequestInterceptor> url_interceptor;

    std::unique_ptr<InputInterpreter> input_interpreter;

    std::thread input_thread;

    std::atomic<bool> input_thread_running{};

    std::atomic<bool> finished{};

    Service::AM::Applets::WebExitReason exit_reason{
        Service::AM::Applets::WebExitReason::EndButtonPressed};

    std::string last_url{"http://localhost/"};

    bool is_local{};

    QWebEngineProfile* default_profile;
    QWebEngineSettings* global_settings;
};

#endif

class QtWebBrowser final : public QObject, public Core::Frontend::WebBrowserApplet {
    Q_OBJECT

public:
    explicit QtWebBrowser(GMainWindow& parent);
    ~QtWebBrowser() override;

    void OpenLocalWebPage(const std::string& local_url,
                          std::function<void()> extract_romfs_callback_,
                          std::function<void(Service::AM::Applets::WebExitReason, std::string)>
                              callback_) const override;

    void OpenExternalWebPage(const std::string& external_url,
                             std::function<void(Service::AM::Applets::WebExitReason, std::string)>
                                 callback_) const override;

signals:
    void MainWindowOpenWebPage(const std::string& main_url, const std::string& additional_args,
                               bool is_local) const;

private:
    void MainWindowExtractOfflineRomFS();

    void MainWindowWebBrowserClosed(Service::AM::Applets::WebExitReason exit_reason,
                                    std::string last_url);

    mutable std::function<void()> extract_romfs_callback;

    mutable std::function<void(Service::AM::Applets::WebExitReason, std::string)> callback;
};
