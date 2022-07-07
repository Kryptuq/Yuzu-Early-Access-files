// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <numeric>

#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"

namespace AudioCore {
using CpuAddr = std::uintptr_t;

enum class PlayState : u8 {
    Started,
    Stopped,
    Paused,
};

enum class SrcQuality : u8 {
    Medium,
    High,
    Low,
};

enum class SampleFormat : u8 {
    Invalid,
    PcmInt8,
    PcmInt16,
    PcmInt24,
    PcmInt32,
    PcmFloat,
    Adpcm,
};

enum class SessionTypes {
    AudioIn,
    AudioOut,
    FinalOutputRecorder,
};

constexpr u32 BufferCount = 32;

constexpr u32 MaxRendererSessions = 2;
constexpr u32 TargetSampleCount = 240;
constexpr u32 TargetSampleRate = 48'000;
constexpr u32 MaxChannels = 6;
constexpr u32 MaxMixBuffers = 24;
constexpr u32 MaxWaveBuffers = 4;
constexpr s32 LowestVoicePriority = 0xFF;
constexpr s32 HighestVoicePriority = 0;
constexpr u32 BufferAlignment = 0x40;
constexpr u32 WorkbufferAlignment = 0x1000;
constexpr s32 FinalMixId = 0;
constexpr s32 InvalidDistanceFromFinalMix = std::numeric_limits<s32>::min();
constexpr s32 UnusedSplitterId = -1;
constexpr s32 UnusedMixId = std::numeric_limits<s32>::max();
constexpr u32 InvalidNodeId = 0xF0000000;
constexpr s32 InvalidProcessOrder = -1;
constexpr u32 MaxBiquadFilters = 2;
constexpr u32 MaxEffects = 256;

constexpr bool IsChannelCountValid(u16 channel_count) {
    return channel_count <= 6 &&
           (channel_count == 1 || channel_count == 2 || channel_count == 4 || channel_count == 6);
}

constexpr u32 GetSplitterInParamHeaderMagic() {
    return Common::MakeMagic('S', 'N', 'D', 'H');
}

constexpr u32 GetSplitterInfoMagic() {
    return Common::MakeMagic('S', 'N', 'D', 'I');
}

constexpr u32 GetSplitterSendDataMagic() {
    return Common::MakeMagic('S', 'N', 'D', 'D');
}

constexpr size_t GetSampleFormatByteSize(SampleFormat format) {
    switch (format) {
    case SampleFormat::PcmInt8:
        return 1;
    case SampleFormat::PcmInt16:
        return 2;
    case SampleFormat::PcmInt24:
        return 3;
    case SampleFormat::PcmInt32:
    case SampleFormat::PcmFloat:
        return 4;
    default:
        return 2;
    }
}

} // namespace AudioCore
