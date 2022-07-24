// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace Core {
class System;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::PTM {

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system);

} // namespace Service::PTM
