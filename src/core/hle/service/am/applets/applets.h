// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <queue>

#include "common/swap.h"
#include "core/hle/service/kernel_helpers.h"

union Result;

namespace Core {
class System;
}

namespace Core::Frontend {
class CabinetApplet;
class ControllerApplet;
class ECommerceApplet;
class ErrorApplet;
class MiiEditApplet;
class ParentalControlsApplet;
class PhotoViewerApplet;
class ProfileSelectApplet;
class SoftwareKeyboardApplet;
class WebBrowserApplet;
} // namespace Core::Frontend

namespace Kernel {
class KernelCore;
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Service::AM {

class IStorage;

namespace Applets {

enum class AppletId : u32 {
    OverlayDisplay = 0x02,
    QLaunch = 0x03,
    Starter = 0x04,
    Auth = 0x0A,
    Cabinet = 0x0B,
    Controller = 0x0C,
    DataErase = 0x0D,
    Error = 0x0E,
    NetConnect = 0x0F,
    ProfileSelect = 0x10,
    SoftwareKeyboard = 0x11,
    MiiEdit = 0x12,
    Web = 0x13,
    Shop = 0x14,
    PhotoViewer = 0x15,
    Settings = 0x16,
    OfflineWeb = 0x17,
    LoginShare = 0x18,
    WebAuth = 0x19,
    MyPage = 0x1A,
};

enum class LibraryAppletMode : u32 {
    AllForeground = 0,
    Background = 1,
    NoUI = 2,
    BackgroundIndirectDisplay = 3,
    AllForegroundInitiallyHidden = 4,
};

class AppletDataBroker final {
public:
    explicit AppletDataBroker(Core::System& system_, LibraryAppletMode applet_mode_);
    ~AppletDataBroker();

    struct RawChannelData {
        std::vector<std::vector<u8>> normal;
        std::vector<std::vector<u8>> interactive;
    };

    // Retrieves but does not pop the data sent to applet.
    RawChannelData PeekDataToAppletForDebug() const;

    std::shared_ptr<IStorage> PopNormalDataToGame();
    std::shared_ptr<IStorage> PopNormalDataToApplet();

    std::shared_ptr<IStorage> PopInteractiveDataToGame();
    std::shared_ptr<IStorage> PopInteractiveDataToApplet();

    void PushNormalDataFromGame(std::shared_ptr<IStorage>&& storage);
    void PushNormalDataFromApplet(std::shared_ptr<IStorage>&& storage);

    void PushInteractiveDataFromGame(std::shared_ptr<IStorage>&& storage);
    void PushInteractiveDataFromApplet(std::shared_ptr<IStorage>&& storage);

    void SignalStateChanged();

    Kernel::KReadableEvent& GetNormalDataEvent();
    Kernel::KReadableEvent& GetInteractiveDataEvent();
    Kernel::KReadableEvent& GetStateChangedEvent();

private:
    Core::System& system;
    LibraryAppletMode applet_mode;

    KernelHelpers::ServiceContext service_context;

    // Queues are named from applet's perspective

    // PopNormalDataToApplet and PushNormalDataFromGame
    std::deque<std::shared_ptr<IStorage>> in_channel;

    // PopNormalDataToGame and PushNormalDataFromApplet
    std::deque<std::shared_ptr<IStorage>> out_channel;

    // PopInteractiveDataToApplet and PushInteractiveDataFromGame
    std::deque<std::shared_ptr<IStorage>> in_interactive_channel;

    // PopInteractiveDataToGame and PushInteractiveDataFromApplet
    std::deque<std::shared_ptr<IStorage>> out_interactive_channel;

    Kernel::KEvent* state_changed_event;

    // Signaled on PushNormalDataFromApplet
    Kernel::KEvent* pop_out_data_event;

    // Signaled on PushInteractiveDataFromApplet
    Kernel::KEvent* pop_interactive_out_data_event;
};

class Applet {
public:
    explicit Applet(Core::System& system_, LibraryAppletMode applet_mode_);
    virtual ~Applet();

    virtual void Initialize();

    virtual bool TransactionComplete() const = 0;
    virtual Result GetStatus() const = 0;
    virtual void ExecuteInteractive() = 0;
    virtual void Execute() = 0;

    AppletDataBroker& GetBroker() {
        return broker;
    }

    const AppletDataBroker& GetBroker() const {
        return broker;
    }

    LibraryAppletMode GetLibraryAppletMode() const {
        return applet_mode;
    }

    bool IsInitialized() const {
        return initialized;
    }

protected:
    struct CommonArguments {
        u32_le arguments_version;
        u32_le size;
        u32_le library_version;
        u32_le theme_color;
        bool play_startup_sound;
        u64_le system_tick;
    };
    static_assert(sizeof(CommonArguments) == 0x20, "CommonArguments has incorrect size.");

    CommonArguments common_args{};
    AppletDataBroker broker;
    LibraryAppletMode applet_mode;
    bool initialized = false;
};

struct AppletFrontendSet {
    using CabinetApplet = std::unique_ptr<Core::Frontend::CabinetApplet>;
    using ControllerApplet = std::unique_ptr<Core::Frontend::ControllerApplet>;
    using ErrorApplet = std::unique_ptr<Core::Frontend::ErrorApplet>;
    using MiiEdit = std::unique_ptr<Core::Frontend::MiiEditApplet>;
    using ParentalControlsApplet = std::unique_ptr<Core::Frontend::ParentalControlsApplet>;
    using PhotoViewer = std::unique_ptr<Core::Frontend::PhotoViewerApplet>;
    using ProfileSelect = std::unique_ptr<Core::Frontend::ProfileSelectApplet>;
    using SoftwareKeyboard = std::unique_ptr<Core::Frontend::SoftwareKeyboardApplet>;
    using WebBrowser = std::unique_ptr<Core::Frontend::WebBrowserApplet>;

    AppletFrontendSet();
    AppletFrontendSet(CabinetApplet cabinet_applet, ControllerApplet controller_applet,
                      ErrorApplet error_applet, MiiEdit mii_edit_,
                      ParentalControlsApplet parental_controls_applet, PhotoViewer photo_viewer_,
                      ProfileSelect profile_select_, SoftwareKeyboard software_keyboard_,
                      WebBrowser web_browser_);
    ~AppletFrontendSet();

    AppletFrontendSet(const AppletFrontendSet&) = delete;
    AppletFrontendSet& operator=(const AppletFrontendSet&) = delete;

    AppletFrontendSet(AppletFrontendSet&&) noexcept;
    AppletFrontendSet& operator=(AppletFrontendSet&&) noexcept;

    CabinetApplet cabinet;
    ControllerApplet controller;
    ErrorApplet error;
    MiiEdit mii_edit;
    ParentalControlsApplet parental_controls;
    PhotoViewer photo_viewer;
    ProfileSelect profile_select;
    SoftwareKeyboard software_keyboard;
    WebBrowser web_browser;
};

class AppletManager {
public:
    explicit AppletManager(Core::System& system_);
    ~AppletManager();

    const AppletFrontendSet& GetAppletFrontendSet() const;

    void SetAppletFrontendSet(AppletFrontendSet set);
    void SetDefaultAppletFrontendSet();
    void SetDefaultAppletsIfMissing();
    void ClearAll();

    std::shared_ptr<Applet> GetApplet(AppletId id, LibraryAppletMode mode) const;

private:
    AppletFrontendSet frontend;
    Core::System& system;
};

} // namespace Applets
} // namespace Service::AM
