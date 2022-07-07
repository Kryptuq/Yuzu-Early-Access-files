// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace AudioCore {

struct WaveBufferVersion1 {
    CpuAddr buffer;
    u64 buffer_size;
    u32 start_offset;
    u32 end_offset;
    bool loop;
    bool stream_ended;
    CpuAddr context;
    u64 context_size;
};

struct WaveBufferVersion2 {
    CpuAddr buffer;
    CpuAddr context;
    u64 buffer_size;
    u64 context_size;
    u32 start_offset;
    u32 end_offset;
    u32 loop_start_offset;
    u32 loop_end_offset;
    s32 loop_count;
    bool loop;
    bool stream_ended;
};

} // namespace AudioCore
