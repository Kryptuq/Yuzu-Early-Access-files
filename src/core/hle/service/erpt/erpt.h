// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::ERPT {

/// Registers all ERPT services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& sm, Core::System& system);

} // namespace Service::ERPT
