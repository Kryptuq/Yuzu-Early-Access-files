// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/renderer/performance/performance_entry.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {

enum class PerformanceDetailType : u8 {
    /*  0 */ Invalid,
    /*  1 */ Unk1,
    /*  2 */ Unk2,
    /*  3 */ Unk3,
    /*  4 */ Unk4,
    /*  5 */ Unk5,
    /*  6 */ Unk6,
    /*  7 */ Unk7,
    /*  8 */ Unk8,
    /*  9 */ Unk9,
    /* 10 */ Unk10,
    /* 11 */ Unk11,
    /* 12 */ Unk12,
};

struct PerformanceDetailVersion1 {
    /* 0x00 */ u32 node_id;
    /* 0x04 */ u32 start_time;
    /* 0x08 */ u32 processed_time;
    /* 0x0C */ PerformanceDetailType detail_type;
    /* 0x0D */ PerformanceEntryType entry_type;
};
static_assert(sizeof(PerformanceDetailVersion1) == 0x10,
              "PerformanceDetailVersion1 has the worng size!");

struct PerformanceDetailVersion2 {
    /* 0x00 */ u32 node_id;
    /* 0x04 */ u32 start_time;
    /* 0x08 */ u32 processed_time;
    /* 0x0C */ PerformanceDetailType detail_type;
    /* 0x0D */ PerformanceEntryType entry_type;
    /* 0x10 */ u32 unk_10;
    /* 0x14 */ char unk14[0x4];
};
static_assert(sizeof(PerformanceDetailVersion2) == 0x18,
              "PerformanceDetailVersion2 has the worng size!");

} // namespace AudioCore::AudioRenderer
