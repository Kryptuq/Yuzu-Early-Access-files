// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Service::SM {
class ServiceManager;
}

namespace Core {
class System;
};

namespace Service::BTM {

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system);

} // namespace Service::BTM
