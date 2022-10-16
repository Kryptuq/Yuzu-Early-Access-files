// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Kernel {

class KernelCore;

namespace KInterruptManager {
void HandleInterrupt(KernelCore& kernel, s32 core_id);
void SendInterProcessorInterrupt(KernelCore& kernel, u64 core_mask);

} // namespace KInterruptManager

} // namespace Kernel
