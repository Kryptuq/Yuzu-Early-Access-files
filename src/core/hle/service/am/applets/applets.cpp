// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/assert.h"
#include "core/core.h"
#include "core/frontend/applets/controller.h"
#include "core/frontend/applets/error.h"
#include "core/frontend/applets/general_frontend.h"
#include "core/frontend/applets/mii_edit.h"
#include "core/frontend/applets/profile_select.h"
#include "core/frontend/applets/software_keyboard.h"
#include "core/frontend/applets/web_browser.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applet_ae.h"
#include "core/hle/service/am/applet_oe.h"
#include "core/hle/service/am/applets/applet_controller.h"
#include "core/hle/service/am/applets/applet_error.h"
#include "core/hle/service/am/applets/applet_general_backend.h"
#include "core/hle/service/am/applets/applet_mii_edit.h"
#include "core/hle/service/am/applets/applet_profile_select.h"
#include "core/hle/service/am/applets/applet_software_keyboard.h"
#include "core/hle/service/am/applets/applet_web_browser.h"
#include "core/hle/service/am/applets/applets.h"
#include "core/hle/service/sm/sm.h"

namespace Service::AM::Applets {

AppletDataBroker::AppletDataBroker(Core::System& system_, LibraryAppletMode applet_mode_)
    : system{system_}, applet_mode{applet_mode_}, service_context{system,
                                                                  "ILibraryAppletAccessor"} {
    state_changed_event = service_context.CreateEvent("ILibraryAppletAccessor:StateChangedEvent");
    pop_out_data_event = service_context.CreateEvent("ILibraryAppletAccessor:PopDataOutEvent");
    pop_interactive_out_data_event =
        service_context.CreateEvent("ILibraryAppletAccessor:PopInteractiveDataOutEvent");
}

AppletDataBroker::~AppletDataBroker() {
    service_context.CloseEvent(state_changed_event);
    service_context.CloseEvent(pop_out_data_event);
    service_context.CloseEvent(pop_interactive_out_data_event);
}

AppletDataBroker::RawChannelData AppletDataBroker::PeekDataToAppletForDebug() const {
    std::vector<std::vector<u8>> out_normal;

    for (const auto& storage : in_channel) {
        out_normal.push_back(storage->GetData());
    }

    std::vector<std::vector<u8>> out_interactive;

    for (const auto& storage : in_interactive_channel) {
        out_interactive.push_back(storage->GetData());
    }

    return {std::move(out_normal), std::move(out_interactive)};
}

std::shared_ptr<IStorage> AppletDataBroker::PopNormalDataToGame() {
    if (out_channel.empty())
        return nullptr;

    auto out = std::move(out_channel.front());
    out_channel.pop_front();
    pop_out_data_event->GetWritableEvent().Clear();
    return out;
}

std::shared_ptr<IStorage> AppletDataBroker::PopNormalDataToApplet() {
    if (in_channel.empty())
        return nullptr;

    auto out = std::move(in_channel.front());
    in_channel.pop_front();
    return out;
}

std::shared_ptr<IStorage> AppletDataBroker::PopInteractiveDataToGame() {
    if (out_interactive_channel.empty())
        return nullptr;

    auto out = std::move(out_interactive_channel.front());
    out_interactive_channel.pop_front();
    pop_interactive_out_data_event->GetWritableEvent().Clear();
    return out;
}

std::shared_ptr<IStorage> AppletDataBroker::PopInteractiveDataToApplet() {
    if (in_interactive_channel.empty())
        return nullptr;

    auto out = std::move(in_interactive_channel.front());
    in_interactive_channel.pop_front();
    return out;
}

void AppletDataBroker::PushNormalDataFromGame(std::shared_ptr<IStorage>&& storage) {
    in_channel.emplace_back(std::move(storage));
}

void AppletDataBroker::PushNormalDataFromApplet(std::shared_ptr<IStorage>&& storage) {
    out_channel.emplace_back(std::move(storage));
    pop_out_data_event->GetWritableEvent().Signal();
}

void AppletDataBroker::PushInteractiveDataFromGame(std::shared_ptr<IStorage>&& storage) {
    in_interactive_channel.emplace_back(std::move(storage));
}

void AppletDataBroker::PushInteractiveDataFromApplet(std::shared_ptr<IStorage>&& storage) {
    out_interactive_channel.emplace_back(std::move(storage));
    pop_interactive_out_data_event->GetWritableEvent().Signal();
}

void AppletDataBroker::SignalStateChanged() {
    state_changed_event->GetWritableEvent().Signal();

    switch (applet_mode) {
    case LibraryAppletMode::AllForeground:
    case LibraryAppletMode::AllForegroundInitiallyHidden: {
        auto applet_oe = system.ServiceManager().GetService<AppletOE>("appletOE");
        auto applet_ae = system.ServiceManager().GetService<AppletAE>("appletAE");

        if (applet_oe) {
            applet_oe->GetMessageQueue()->FocusStateChanged();
            break;
        }

        if (applet_ae) {
            applet_ae->GetMessageQueue()->FocusStateChanged();
            break;
        }
        break;
    }
    default:
        break;
    }
}

Kernel::KReadableEvent& AppletDataBroker::GetNormalDataEvent() {
    return pop_out_data_event->GetReadableEvent();
}

Kernel::KReadableEvent& AppletDataBroker::GetInteractiveDataEvent() {
    return pop_interactive_out_data_event->GetReadableEvent();
}

Kernel::KReadableEvent& AppletDataBroker::GetStateChangedEvent() {
    return state_changed_event->GetReadableEvent();
}

Applet::Applet(Core::System& system_, LibraryAppletMode applet_mode_)
    : broker{system_, applet_mode_}, applet_mode{applet_mode_} {}

Applet::~Applet() = default;

void Applet::Initialize() {
    const auto common = broker.PopNormalDataToApplet();
    ASSERT(common != nullptr);

    const auto common_data = common->GetData();

    ASSERT(common_data.size() >= sizeof(CommonArguments));
    std::memcpy(&common_args, common_data.data(), sizeof(CommonArguments));

    initialized = true;
}

AppletFrontendSet::AppletFrontendSet() = default;

AppletFrontendSet::AppletFrontendSet(ControllerApplet controller_applet, ErrorApplet error_applet,
                                     MiiEdit mii_edit_,
                                     ParentalControlsApplet parental_controls_applet,
                                     PhotoViewer photo_viewer_, ProfileSelect profile_select_,
                                     SoftwareKeyboard software_keyboard_, WebBrowser web_browser_)
    : controller{std::move(controller_applet)}, error{std::move(error_applet)},
      mii_edit{std::move(mii_edit_)}, parental_controls{std::move(parental_controls_applet)},
      photo_viewer{std::move(photo_viewer_)}, profile_select{std::move(profile_select_)},
      software_keyboard{std::move(software_keyboard_)}, web_browser{std::move(web_browser_)} {}

AppletFrontendSet::~AppletFrontendSet() = default;

AppletFrontendSet::AppletFrontendSet(AppletFrontendSet&&) noexcept = default;

AppletFrontendSet& AppletFrontendSet::operator=(AppletFrontendSet&&) noexcept = default;

AppletManager::AppletManager(Core::System& system_) : system{system_} {}

AppletManager::~AppletManager() = default;

const AppletFrontendSet& AppletManager::GetAppletFrontendSet() const {
    return frontend;
}

void AppletManager::SetAppletFrontendSet(AppletFrontendSet set) {
    if (set.controller != nullptr) {
        frontend.controller = std::move(set.controller);
    }

    if (set.error != nullptr) {
        frontend.error = std::move(set.error);
    }

    if (set.mii_edit != nullptr) {
        frontend.mii_edit = std::move(set.mii_edit);
    }

    if (set.parental_controls != nullptr) {
        frontend.parental_controls = std::move(set.parental_controls);
    }

    if (set.photo_viewer != nullptr) {
        frontend.photo_viewer = std::move(set.photo_viewer);
    }

    if (set.profile_select != nullptr) {
        frontend.profile_select = std::move(set.profile_select);
    }

    if (set.software_keyboard != nullptr) {
        frontend.software_keyboard = std::move(set.software_keyboard);
    }

    if (set.web_browser != nullptr) {
        frontend.web_browser = std::move(set.web_browser);
    }
}

void AppletManager::SetDefaultAppletFrontendSet() {
    ClearAll();
    SetDefaultAppletsIfMissing();
}

void AppletManager::SetDefaultAppletsIfMissing() {
    if (frontend.controller == nullptr) {
        frontend.controller =
            std::make_unique<Core::Frontend::DefaultControllerApplet>(system.HIDCore());
    }

    if (frontend.error == nullptr) {
        frontend.error = std::make_unique<Core::Frontend::DefaultErrorApplet>();
    }

    if (frontend.mii_edit == nullptr) {
        frontend.mii_edit = std::make_unique<Core::Frontend::DefaultMiiEditApplet>();
    }

    if (frontend.parental_controls == nullptr) {
        frontend.parental_controls =
            std::make_unique<Core::Frontend::DefaultParentalControlsApplet>();
    }

    if (frontend.photo_viewer == nullptr) {
        frontend.photo_viewer = std::make_unique<Core::Frontend::DefaultPhotoViewerApplet>();
    }

    if (frontend.profile_select == nullptr) {
        frontend.profile_select = std::make_unique<Core::Frontend::DefaultProfileSelectApplet>();
    }

    if (frontend.software_keyboard == nullptr) {
        frontend.software_keyboard =
            std::make_unique<Core::Frontend::DefaultSoftwareKeyboardApplet>();
    }

    if (frontend.web_browser == nullptr) {
        frontend.web_browser = std::make_unique<Core::Frontend::DefaultWebBrowserApplet>();
    }
}

void AppletManager::ClearAll() {
    frontend = {};
}

std::shared_ptr<Applet> AppletManager::GetApplet(AppletId id, LibraryAppletMode mode) const {
    switch (id) {
    case AppletId::Auth:
        return std::make_shared<Auth>(system, mode, *frontend.parental_controls);
    case AppletId::Controller:
        return std::make_shared<Controller>(system, mode, *frontend.controller);
    case AppletId::Error:
        return std::make_shared<Error>(system, mode, *frontend.error);
    case AppletId::ProfileSelect:
        return std::make_shared<ProfileSelect>(system, mode, *frontend.profile_select);
    case AppletId::SoftwareKeyboard:
        return std::make_shared<SoftwareKeyboard>(system, mode, *frontend.software_keyboard);
    case AppletId::MiiEdit:
        return std::make_shared<MiiEdit>(system, mode, *frontend.mii_edit);
    case AppletId::Web:
    case AppletId::Shop:
    case AppletId::OfflineWeb:
    case AppletId::LoginShare:
    case AppletId::WebAuth:
        return std::make_shared<WebBrowser>(system, mode, *frontend.web_browser);
    case AppletId::PhotoViewer:
        return std::make_shared<PhotoViewer>(system, mode, *frontend.photo_viewer);
    default:
        UNIMPLEMENTED_MSG(
            "No backend implementation exists for applet_id={:02X}! Falling back to stub applet.",
            static_cast<u8>(id));
        return std::make_shared<StubApplet>(system, id, mode);
    }
}

} // namespace Service::AM::Applets
