// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <numeric>
#include <span>

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

enum class Channels : u32 {
    FrontLeft,
    FrontRight,
    Center,
    LFE,
    BackLeft,
    BackRight,
};

// These are used by Delay, Reverb and I3dl2Reverb prior to Revision 11.
enum class OldChannels : u32 {
    FrontLeft,
    FrontRight,
    BackLeft,
    BackRight,
    Center,
    LFE,
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

constexpr void UseOldChannelMapping(std::span<s16> inputs, std::span<s16> outputs) {
    constexpr auto old_center{static_cast<u32>(OldChannels::Center)};
    constexpr auto new_center{static_cast<u32>(Channels::Center)};
    constexpr auto old_lfe{static_cast<u32>(OldChannels::LFE)};
    constexpr auto new_lfe{static_cast<u32>(Channels::LFE)};

    auto center{inputs[old_center]};
    auto lfe{inputs[old_lfe]};
    inputs[old_center] = inputs[new_center];
    inputs[old_lfe] = inputs[new_lfe];
    inputs[new_center] = center;
    inputs[new_lfe] = lfe;

    center = outputs[old_center];
    lfe = outputs[old_lfe];
    outputs[old_center] = outputs[new_center];
    outputs[old_lfe] = outputs[new_lfe];
    outputs[new_center] = center;
    outputs[new_lfe] = lfe;
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
