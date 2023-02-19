// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
} // namespace Core

namespace Service::Glue {

void LoopProcess(Core::System& system);

} // namespace Service::Glue
