// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Kernel {

class KernelCore;

namespace KInterruptManager {
void HandleInterrupt(KernelCore& kernel, s32 core_id);
}

} // namespace Kernel
