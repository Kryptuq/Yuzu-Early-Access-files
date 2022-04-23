// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Tegra::Engines {

struct ConstBufferInfo {
    GPUVAddr address;
    u32 size;
    bool enabled;
};

} // namespace Tegra::Engines
