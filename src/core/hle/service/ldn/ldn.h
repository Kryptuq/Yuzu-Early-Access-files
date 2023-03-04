// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_event.h"
#include "core/hle/result.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/sm/sm.h"

namespace Core {
class System;
}

namespace Service::LDN {

void LoopProcess(Core::System& system);

} // namespace Service::LDN
