// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace AudioCore::AudioRenderer {

enum class PerformanceEntryType : u8 {
    Invalid,
    Voice,
    SubMix,
    FinalMix,
    Sink,
};

struct PerformanceEntryVersion1 {
    /* 0x00 */ u32 node_id;
    /* 0x04 */ u32 start_time;
    /* 0x08 */ u32 processed_time;
    /* 0x0C */ PerformanceEntryType entry_type;
};
static_assert(sizeof(PerformanceEntryVersion1) == 0x10,
              "PerformanceEntryVersion1 has the wrong size!");

struct PerformanceEntryVersion2 {
    /* 0x00 */ u32 node_id;
    /* 0x04 */ u32 start_time;
    /* 0x08 */ u32 processed_time;
    /* 0x0C */ PerformanceEntryType entry_type;
    /* 0x0D */ char unk0D[0xB];
};
static_assert(sizeof(PerformanceEntryVersion2) == 0x18,
              "PerformanceEntryVersion2 has the wrong size!");

} // namespace AudioCore::AudioRenderer
