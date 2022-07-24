// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::JIT {

/// Registers all JIT services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& sm, Core::System& system);

} // namespace Service::JIT
