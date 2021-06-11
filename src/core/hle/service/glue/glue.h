// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Core {
class System;
} // namespace Core

namespace Service::Glue {

/// Registers all Glue services with the specified service manager.
void InstallInterfaces(Core::System& system);

} // namespace Service::Glue
