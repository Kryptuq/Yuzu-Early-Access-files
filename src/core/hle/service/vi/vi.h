// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace Service::NVFlinger {
class NVFlinger;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::VI {

enum class DisplayResolution : u32 {
    DockedWidth = 1920,
    DockedHeight = 1080,
    UndockedWidth = 1280,
    UndockedHeight = 720,
};

/// Permission level for a particular VI service instance
enum class Permission {
    User,
    System,
    Manager,
};

/// A policy type that may be requested via GetDisplayService and
/// GetDisplayServiceWithProxyNameExchange
enum class Policy {
    User,
    Compositor,
};

namespace detail {
void GetDisplayServiceImpl(Kernel::HLERequestContext& ctx, Core::System& system,
                           NVFlinger::NVFlinger& nv_flinger, Permission permission);
} // namespace detail

/// Registers all VI services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system,
                       NVFlinger::NVFlinger& nv_flinger);

} // namespace Service::VI
