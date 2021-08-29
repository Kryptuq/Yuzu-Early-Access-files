// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

namespace Core {
class System;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::NGCT {

/// Registers all NGCT services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::NGCT
