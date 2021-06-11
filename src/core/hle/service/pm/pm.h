// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Core {
class System;
}

namespace Service::PM {

enum class SystemBootMode {
    Normal,
    Maintenance,
};

/// Registers all PM services with the specified service manager.
void InstallInterfaces(Core::System& system);

} // namespace Service::PM
