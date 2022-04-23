// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::PlayReport {

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::PlayReport
