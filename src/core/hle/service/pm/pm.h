// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
}

namespace Service::PM {

enum class SystemBootMode {
    Normal,
    Maintenance,
};

void LoopProcess(Core::System& system);

} // namespace Service::PM
