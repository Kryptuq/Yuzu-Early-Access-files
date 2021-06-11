// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/am/applets/applets.h"

namespace Core {
class System;
}

namespace Service::AM::Applets {

enum class AuthAppletType : u32 {
    ShowParentalAuthentication,
    RegisterParentalPasscode,
    ChangeParentalPasscode,
};

class Auth final : public Applet {
public:
    explicit Auth(Core::System& system_, LibraryAppletMode applet_mode_,
                  Core::Frontend::ParentalControlsApplet& frontend_);
    ~Auth() override;

    void Initialize() override;
    bool TransactionComplete() const override;
    ResultCode GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;

    void AuthFinished(bool is_successful = true);

private:
    Core::Frontend::ParentalControlsApplet& frontend;
    Core::System& system;
    bool complete = false;
    bool successful = false;

    AuthAppletType type = AuthAppletType::ShowParentalAuthentication;
    u8 arg0 = 0;
    u8 arg1 = 0;
    u8 arg2 = 0;
};

enum class PhotoViewerAppletMode : u8 {
    CurrentApp = 0,
    AllApps = 1,
};

class PhotoViewer final : public Applet {
public:
    explicit PhotoViewer(Core::System& system_, LibraryAppletMode applet_mode_,
                         const Core::Frontend::PhotoViewerApplet& frontend_);
    ~PhotoViewer() override;

    void Initialize() override;
    bool TransactionComplete() const override;
    ResultCode GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;

    void ViewFinished();

private:
    const Core::Frontend::PhotoViewerApplet& frontend;
    bool complete = false;
    PhotoViewerAppletMode mode = PhotoViewerAppletMode::CurrentApp;
    Core::System& system;
};

class StubApplet final : public Applet {
public:
    explicit StubApplet(Core::System& system_, AppletId id_, LibraryAppletMode applet_mode_);
    ~StubApplet() override;

    void Initialize() override;

    bool TransactionComplete() const override;
    ResultCode GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;

private:
    AppletId id;
    Core::System& system;
};

} // namespace Service::AM::Applets
