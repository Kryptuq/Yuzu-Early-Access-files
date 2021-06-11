// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Tegra::Engines {

struct ConstBufferInfo {
    GPUVAddr address;
    u32 size;
    bool enabled;
};

} // namespace Tegra::Engines
