// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Kernel::Board::Nintendo::Nx::Smc {

enum MemorySize {
    MemorySize_4GB = 0,
    MemorySize_6GB = 1,
    MemorySize_8GB = 2,
};

enum MemoryArrangement {
    MemoryArrangement_4GB = 0,
    MemoryArrangement_4GBForAppletDev = 1,
    MemoryArrangement_4GBForSystemDev = 2,
    MemoryArrangement_6GB = 3,
    MemoryArrangement_6GBForAppletDev = 4,
    MemoryArrangement_8GB = 5,
};

} // namespace Kernel::Board::Nintendo::Nx::Smc
