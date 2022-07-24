// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/common/common.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer::ADSP {

struct CommandBuffer {
    CpuAddr buffer;
    u64 size;
    u64 time_limit;
    u32 remaining_command_count;
    bool reset_buffers;
    u64 applet_resource_user_id;
    u64 render_time_taken;
};

} // namespace AudioCore::AudioRenderer::ADSP
