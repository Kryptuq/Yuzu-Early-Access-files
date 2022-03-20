// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2012 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/ui/Fence.h

#pragma once

#include <array>

#include "common/common_types.h"
#include "core/hle/service/nvdrv/nvdata.h"

namespace Service::android {

class Fence {
public:
    constexpr Fence() = default;

    static constexpr Fence NoFence() {
        Fence fence;
        fence.fences[0].id = -1;
        return fence;
    }

public:
    u32 num_fences{};
    std::array<Service::Nvidia::NvFence, 4> fences{};
};
static_assert(sizeof(Fence) == 36, "Fence has wrong size");

} // namespace Service::android
