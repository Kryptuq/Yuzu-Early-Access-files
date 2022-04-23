// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::NPNS {

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system);

} // namespace Service::NPNS
