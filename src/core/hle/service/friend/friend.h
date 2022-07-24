// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Friend {

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(std::shared_ptr<Module> module_, Core::System& system_,
                           const char* name);
        ~Interface() override;

        void CreateFriendService(Kernel::HLERequestContext& ctx);
        void CreateNotificationService(Kernel::HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> module;
    };
};

/// Registers all Friend services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::Friend
