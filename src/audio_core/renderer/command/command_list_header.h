// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/common/common.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {

struct CommandListHeader {
    u64 buffer_size;
    u32 command_count;
    std::span<s32> samples_buffer;
    s16 buffer_count;
    u32 sample_count;
    u32 sample_rate;
};

} // namespace AudioCore::AudioRenderer
