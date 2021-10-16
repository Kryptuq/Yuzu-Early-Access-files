// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef YUZU_USE_QT_WEB_ENGINE
#include <QApplication>
#include <QKeyEvent>

#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QWebEngineUrlScheme>
#endif

#include "common/fs/path_util.h"
#include "core/core.h"
#include "core/frontend/input_interpreter.h"
#include "input_common/keyboard.h"
#include "input_common/main.h"
#include "yuzu/applets/qt_web_browser.h"
#include "yuzu/applets/qt_web_browser_scripts.h"
#include "yuzu/main.h"
#include "yuzu/util/url_request_interceptor.h"

#ifdef YUZU_USE_QT_WEB_ENGINE

namespace {

constexpr int HIDButtonToKey(HIDButton button) {
    switch (button) {
    case HIDButton::DLeft:
    case HIDButton::LStickLeft:
        return Qt::Key_Left;
    case HIDButton::DUp:
    case HIDButton::LStickUp:
        return Qt::Key_Up;
    case HIDButton::DRight:
    case HIDButton::LStickRight:
        return Qt::Key_Right;
    case HIDButton::DDown:
    case HIDButton::LStickDown:
        return Qt::Key_Down;
    default:
        return 0;
    }
}

} // Anonymous namespace

QtNXWebEngineView::QtNXWebEngineView(QWidget* parent, Core::System& system,
                                     InputCommon::InputSubsystem* input_subsystem_)
    : QWebEngineView(parent), input_subsystem{input_subsystem_},
      url_interceptor(std::make_unique<UrlRequestInterceptor>()),
      input_interpreter(std::make_unique<InputInterpreter>(system)),
      default_profile{QWebEngineProfile::defaultProfile()},
      global_settings{QWebEngineSettings::globalSettings()} {
    default_profile->setPersistentStoragePath(QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::YuzuDir) / "qtwebengine")));

    QWebEngineScript gamepad;
    QWebEngineScript window_nx;

    gamepad.setName(QStringLiteral("gamepad_script.js"));
    window_nx.setName(QStringLiteral("window_nx_script.js"));

    gamepad.setSourceCode(QString::fromStdString(GAMEPAD_SCRIPT));
    window_nx.setSourceCode(QString::fromStdString(WINDOW_NX_SCRIPT));

    gamepad.setInjectionPoint(QWebEngineScript::DocumentCreation);
    window_nx.setInjectionPoint(QWebEngineScript::DocumentCreation);

    gamepad.setWorldId(QWebEngineScript::MainWorld);
    window_nx.setWorldId(QWebEngineScript::MainWorld);

    gamepad.setRunsOnSubFrames(true);
    window_nx.setRunsOnSubFrames(true);

    default_profile->scripts()->insert(gamepad);
    default_profile->scripts()->insert(window_nx);

    default_profile->setRequestInterceptor(url_interceptor.get());

    global_settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    global_settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    global_settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    global_settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    global_settings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);
    global_settings->setAttribute(QWebEngineSettings::ShowScrollBars, false);

    global_settings->setFontFamily(QWebEngineSettings::StandardFont, QStringLiteral("Roboto"));

    connect(
        page(), &QWebEnginePage::windowCloseRequested, page(),
        [this] {
            if (page()->url() == url_interceptor->GetRequestedURL()) {
                SetFinished(true);
                SetExitReason(Service::AM::Applets::WebExitReason::WindowClosed);
            }
        },
        Qt::QueuedConnection);
}

QtNXWebEngineView::~QtNXWebEngineView() {
    SetFinished(true);
    StopInputThread();
}

void QtNXWebEngineView::LoadLocalWebPage(const std::string& main_url,
                                         const std::string& additional_args) {
    is_local = true;

    LoadExtractedFonts();
    FocusFirstLinkElement();
    SetUserAgent(UserAgent::WebApplet);
    SetFinished(false);
    SetExitReason(Service::AM::Applets::WebExitReason::EndButtonPressed);
    SetLastURL("http://localhost/");
    StartInputThread();

    load(QUrl(QUrl::fromLocalFile(QString::fromStdString(main_url)).toString() +
              QString::fromStdString(additional_args)));
}

void QtNXWebEngineView::LoadExternalWebPage(const std::string& main_url,
                                            const std::string& additional_args) {
    is_local = false;

    FocusFirstLinkElement();
    SetUserAgent(UserAgent::WebApplet);
    SetFinished(false);
    SetExitReason(Service::AM::Applets::WebExitReason::EndButtonPressed);
    SetLastURL("http://localhost/");
    StartInputThread();

    load(QUrl(QString::fromStdString(main_url) + QString::fromStdString(additional_args)));
}

void QtNXWebEngineView::SetUserAgent(UserAgent user_agent) {
    const QString user_agent_str = [user_agent] {
        switch (user_agent) {
        case UserAgent::WebApplet:
        default:
            return QStringLiteral("WebApplet");
        case UserAgent::ShopN:
            return QStringLiteral("ShopN");
        case UserAgent::LoginApplet:
            return QStringLiteral("LoginApplet");
        case UserAgent::ShareApplet:
            return QStringLiteral("ShareApplet");
        case UserAgent::LobbyApplet:
            return QStringLiteral("LobbyApplet");
        case UserAgent::WifiWebAuthApplet:
            return QStringLiteral("WifiWebAuthApplet");
        }
    }();

    QWebEngineProfile::defaultProfile()->setHttpUserAgent(
        QStringLiteral("Mozilla/5.0 (Nintendo Switch; %1) AppleWebKit/606.4 "
                       "(KHTML, like Gecko) NF/6.0.1.15.4 NintendoBrowser/5.1.0.20389")
            .arg(user_agent_str));
}

bool QtNXWebEngineView::IsFinished() const {
    return finished;
}

void QtNXWebEngineView::SetFinished(bool finished_) {
    finished = finished_;
}

Service::AM::Applets::WebExitReason QtNXWebEngineView::GetExitReason() const {
    return exit_reason;
}

void QtNXWebEngineView::SetExitReason(Service::AM::Applets::WebExitReason exit_reason_) {
    exit_reason = exit_reason_;
}

const std::string& QtNXWebEngineView::GetLastURL() const {
    return last_url;
}

void QtNXWebEngineView::SetLastURL(std::string last_url_) {
    last_url = std::move(last_url_);
}

QString QtNXWebEngineView::GetCurrentURL() const {
    return url_interceptor->GetRequestedURL().toString();
}

void QtNXWebEngineView::hide() {
    SetFinished(true);
    StopInputThread();

    QWidget::hide();
}

void QtNXWebEngineView::keyPressEvent(QKeyEvent* event) {
    if (is_local) {
        input_subsystem->GetKeyboard()->PressKey(event->key());
    }
}

void QtNXWebEngineView::keyReleaseEvent(QKeyEvent* event) {
    if (is_local) {
        input_subsystem->GetKeyboard()->ReleaseKey(event->key());
    }
}

template <HIDButton... T>
void QtNXWebEngineView::HandleWindowFooterButtonPressedOnce() {
    const auto f = [this](HIDButton button) {
        if (input_interpreter->IsButtonPressedOnce(button)) {
            page()->runJavaScript(
                QStringLiteral("yuzu_key_callbacks[%1] == null;").arg(static_cast<u8>(button)),
                [this, button](const QVariant& variant) {
                    if (variant.toBool()) {
                        switch (button) {
                        case HIDButton::A:
                            SendMultipleKeyPressEvents<Qt::Key_A, Qt::Key_Space, Qt::Key_Return>();
                            break;
                        case HIDButton::B:
                            SendKeyPressEvent(Qt::Key_B);
                            break;
                        case HIDButton::X:
                            SendKeyPressEvent(Qt::Key_X);
                            break;
                        case HIDButton::Y:
                            SendKeyPressEvent(Qt::Key_Y);
                            break;
                        default:
                            break;
                        }
                    }
                });

            page()->runJavaScript(
                QStringLiteral("if (yuzu_key_callbacks[%1] != null) { yuzu_key_callbacks[%1](); }")
                    .arg(static_cast<u8>(button)));
        }
    };

    (f(T), ...);
}

template <HIDButton... T>
void QtNXWebEngineView::HandleWindowKeyButtonPressedOnce() {
    const auto f = [this](HIDButton button) {
        if (input_interpreter->IsButtonPressedOnce(button)) {
            SendKeyPressEvent(HIDButtonToKey(button));
        }
    };

    (f(T), ...);
}

template <HIDButton... T>
void QtNXWebEngineView::HandleWindowKeyButtonHold() {
    const auto f = [this](HIDButton button) {
        if (input_interpreter->IsButtonHeld(button)) {
            SendKeyPressEvent(HIDButtonToKey(button));
        }
    };

    (f(T), ...);
}

void QtNXWebEngineView::SendKeyPressEvent(int key) {
    if (key == 0) {
        return;
    }

    QCoreApplication::postEvent(focusProxy(),
                                new QKeyEvent(QKeyEvent::KeyPress, key, Qt::NoModifier));
    QCoreApplication::postEvent(focusProxy(),
                                new QKeyEvent(QKeyEvent::KeyRelease, key, Qt::NoModifier));
}

void QtNXWebEngineView::StartInputThread() {
    if (input_thread_running) {
        return;
    }

    input_thread_running = true;
    input_thread = std::thread(&QtNXWebEngineView::InputThread, this);
}

void QtNXWebEngineView::StopInputThread() {
    if (is_local) {
        QWidget::releaseKeyboard();
    }

    input_thread_running = false;
    if (input_thread.joinable()) {
        input_thread.join();
    }
}

void QtNXWebEngineView::InputThread() {
    // Wait for 1 second before allowing any inputs to be processed.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (is_local) {
        QWidget::grabKeyboard();
    }

    while (input_thread_running) {
        input_interpreter->PollInput();

        HandleWindowFooterButtonPressedOnce<HIDButton::A, HIDButton::B, HIDButton::X, HIDButton::Y,
                                            HIDButton::L, HIDButton::R>();

        HandleWindowKeyButtonPressedOnce<HIDButton::DLeft, HIDButton::DUp, HIDButton::DRight,
                                         HIDButton::DDown, HIDButton::LStickLeft,
                                         HIDButton::LStickUp, HIDButton::LStickRight,
                                         HIDButton::LStickDown>();

        HandleWindowKeyButtonHold<HIDButton::DLeft, HIDButton::DUp, HIDButton::DRight,
                                  HIDButton::DDown, HIDButton::LStickLeft, HIDButton::LStickUp,
                                  HIDButton::LStickRight, HIDButton::LStickDown>();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void QtNXWebEngineView::LoadExtractedFonts() {
    QWebEngineScript nx_font_css;
    QWebEngineScript load_nx_font;

    auto fonts_dir_str = Common::FS::PathToUTF8String(
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::CacheDir) / "fonts/");

    std::replace(fonts_dir_str.begin(), fonts_dir_str.end(), '\\', '/');

    const auto fonts_dir = QString::fromStdString(fonts_dir_str);

    nx_font_css.setName(QStringLiteral("nx_font_css.js"));
    load_nx_font.setName(QStringLiteral("load_nx_font.js"));

    nx_font_css.setSourceCode(
        QString::fromStdString(NX_FONT_CSS)
            .arg(fonts_dir + QStringLiteral("FontStandard.ttf"))
            .arg(fonts_dir + QStringLiteral("FontChineseSimplified.ttf"))
            .arg(fonts_dir + QStringLiteral("FontExtendedChineseSimplified.ttf"))
            .arg(fonts_dir + QStringLiteral("FontChineseTraditional.ttf"))
            .arg(fonts_dir + QStringLiteral("FontKorean.ttf"))
            .arg(fonts_dir + QStringLiteral("FontNintendoExtended.ttf"))
            .arg(fonts_dir + QStringLiteral("FontNintendoExtended2.ttf")));
    load_nx_font.setSourceCode(QString::fromStdString(LOAD_NX_FONT));

    nx_font_css.setInjectionPoint(QWebEngineScript::DocumentReady);
    load_nx_font.setInjectionPoint(QWebEngineScript::Deferred);

    nx_font_css.setWorldId(QWebEngineScript::MainWorld);
    load_nx_font.setWorldId(QWebEngineScript::MainWorld);

    nx_font_css.setRunsOnSubFrames(true);
    load_nx_font.setRunsOnSubFrames(true);

    default_profile->scripts()->insert(nx_font_css);
    default_profile->scripts()->insert(load_nx_font);

    connect(
        url_interceptor.get(), &UrlRequestInterceptor::FrameChanged, url_interceptor.get(),
        [this] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            page()->runJavaScript(QString::fromStdString(LOAD_NX_FONT));
        },
        Qt::QueuedConnection);
}

void QtNXWebEngineView::FocusFirstLinkElement() {
    QWebEngineScript focus_link_element;

    focus_link_element.setName(QStringLiteral("focus_link_element.js"));
    focus_link_element.setSourceCode(QString::fromStdString(FOCUS_LINK_ELEMENT_SCRIPT));
    focus_link_element.setWorldId(QWebEngineScript::MainWorld);
    focus_link_element.setInjectionPoint(QWebEngineScript::Deferred);
    focus_link_element.setRunsOnSubFrames(true);
    default_profile->scripts()->insert(focus_link_element);
}

#endif

QtWebBrowser::QtWebBrowser(GMainWindow& main_window) {
    connect(this, &QtWebBrowser::MainWindowOpenWebPage, &main_window,
            &GMainWindow::WebBrowserOpenWebPage, Qt::QueuedConnection);
    connect(&main_window, &GMainWindow::WebBrowserExtractOfflineRomFS, this,
            &QtWebBrowser::MainWindowExtractOfflineRomFS, Qt::QueuedConnection);
    connect(&main_window, &GMainWindow::WebBrowserClosed, this,
            &QtWebBrowser::MainWindowWebBrowserClosed, Qt::QueuedConnection);
}

QtWebBrowser::~QtWebBrowser() = default;

void QtWebBrowser::OpenLocalWebPage(
    const std::string& local_url, std::function<void()> extract_romfs_callback_,
    std::function<void(Service::AM::Applets::WebExitReason, std::string)> callback_) const {
    extract_romfs_callback = std::move(extract_romfs_callback_);
    callback = std::move(callback_);

    const auto index = local_url.find('?');

    if (index == std::string::npos) {
        emit MainWindowOpenWebPage(local_url, "", true);
    } else {
        emit MainWindowOpenWebPage(local_url.substr(0, index), local_url.substr(index), true);
    }
}

void QtWebBrowser::OpenExternalWebPage(
    const std::string& external_url,
    std::function<void(Service::AM::Applets::WebExitReason, std::string)> callback_) const {
    callback = std::move(callback_);

    const auto index = external_url.find('?');

    if (index == std::string::npos) {
        emit MainWindowOpenWebPage(external_url, "", false);
    } else {
        emit MainWindowOpenWebPage(external_url.substr(0, index), external_url.substr(index),
                                   false);
    }
}

void QtWebBrowser::MainWindowExtractOfflineRomFS() {
    extract_romfs_callback();
}

void QtWebBrowser::MainWindowWebBrowserClosed(Service::AM::Applets::WebExitReason exit_reason,
                                              std::string last_url) {
    callback(exit_reason, last_url);
}
