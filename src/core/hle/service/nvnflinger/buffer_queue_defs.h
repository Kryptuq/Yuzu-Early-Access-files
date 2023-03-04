// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2014 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/BufferQueueDefs.h

#pragma once

#include <array>

#include "common/common_types.h"
#include "core/hle/service/nvnflinger/buffer_slot.h"

namespace Service::android::BufferQueueDefs {

// BufferQueue will keep track of at most this value of buffers.
constexpr s32 NUM_BUFFER_SLOTS = 64;

using SlotsType = std::array<BufferSlot, NUM_BUFFER_SLOTS>;

} // namespace Service::android::BufferQueueDefs
