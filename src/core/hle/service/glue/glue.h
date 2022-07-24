// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
} // namespace Core

namespace Service::Glue {

/// Registers all Glue services with the specified service manager.
void InstallInterfaces(Core::System& system);

} // namespace Service::Glue
