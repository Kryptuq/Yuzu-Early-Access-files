// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Core {
class System;
}

namespace Service {
class HLERequestContext;
}

namespace Service::Nvnflinger {
class HosBinderDriverServer;
class Nvnflinger;
} // namespace Service::Nvnflinger

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
void GetDisplayServiceImpl(HLERequestContext& ctx, Core::System& system,
                           Nvnflinger::Nvnflinger& nv_flinger,
                           Nvnflinger::HosBinderDriverServer& hos_binder_driver_server,
                           Permission permission);
} // namespace detail

void LoopProcess(Core::System& system, Nvnflinger::Nvnflinger& nv_flinger,
                 Nvnflinger::HosBinderDriverServer& hos_binder_driver_server);

} // namespace Service::VI
