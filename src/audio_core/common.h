// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/result.h"

namespace AudioCommon {
namespace Audren {
constexpr ResultCode ERR_INVALID_PARAMETERS{ErrorModule::Audio, 41};
constexpr ResultCode ERR_SPLITTER_SORT_FAILED{ErrorModule::Audio, 43};
} // namespace Audren

constexpr u32_le CURRENT_PROCESS_REVISION = Common::MakeMagic('R', 'E', 'V', '9');
constexpr std::size_t MAX_MIX_BUFFERS = 24;
constexpr std::size_t MAX_BIQUAD_FILTERS = 2;
constexpr std::size_t MAX_CHANNEL_COUNT = 6;
constexpr std::size_t MAX_WAVE_BUFFERS = 4;
constexpr std::size_t MAX_SAMPLE_HISTORY = 4;
constexpr u32 STREAM_SAMPLE_RATE = 48000;
constexpr u32 STREAM_NUM_CHANNELS = 2;
constexpr s32 NO_SPLITTER = -1;
constexpr s32 NO_MIX = 0x7fffffff;
constexpr s32 NO_FINAL_MIX = std::numeric_limits<s32>::min();
constexpr s32 FINAL_MIX = 0;
constexpr s32 NO_EFFECT_ORDER = -1;
constexpr std::size_t TEMP_MIX_BASE_SIZE = 0x3f00; // TODO(ogniK): Work out this constant
// Any size checks seem to take the sample history into account
// and our const ends up being 0x3f04, the 4 bytes are most
// likely the sample history
constexpr std::size_t TOTAL_TEMP_MIX_SIZE = TEMP_MIX_BASE_SIZE + AudioCommon::MAX_SAMPLE_HISTORY;
constexpr f32 I3DL2REVERB_MAX_LEVEL = 5000.0f;
constexpr f32 I3DL2REVERB_MIN_REFLECTION_DURATION = 0.02f;
constexpr std::size_t I3DL2REVERB_TAPS = 20;
constexpr std::size_t I3DL2REVERB_DELAY_LINE_COUNT = 4;
using Fractional = s32;

template <typename T>
constexpr Fractional ToFractional(T x) {
    return static_cast<Fractional>(x * static_cast<T>(0x4000));
}

constexpr Fractional MultiplyFractional(Fractional lhs, Fractional rhs) {
    return static_cast<Fractional>(static_cast<s64>(lhs) * rhs >> 14);
}

constexpr s32 FractionalToFixed(Fractional x) {
    const auto s = x & (1 << 13);
    return static_cast<s32>(x >> 14) + s;
}

constexpr s32 CalculateDelaySamples(s32 sample_rate_khz, float time) {
    return FractionalToFixed(MultiplyFractional(ToFractional(sample_rate_khz), ToFractional(time)));
}

static constexpr u32 VersionFromRevision(u32_le rev) {
    // "REV7" -> 7
    return ((rev >> 24) & 0xff) - 0x30;
}

static constexpr bool IsRevisionSupported(u32 required, u32_le user_revision) {
    const auto base = VersionFromRevision(user_revision);
    return required <= base;
}

static constexpr bool IsValidRevision(u32_le revision) {
    const auto base = VersionFromRevision(revision);
    constexpr auto max_rev = VersionFromRevision(CURRENT_PROCESS_REVISION);
    return base <= max_rev;
}

static constexpr bool CanConsumeBuffer(std::size_t size, std::size_t offset, std::size_t required) {
    if (offset > size) {
        return false;
    }
    if (size < required) {
        return false;
    }
    if ((size - offset) < required) {
        return false;
    }
    return true;
}

struct UpdateDataSizes {
    u32_le behavior{};
    u32_le memory_pool{};
    u32_le voice{};
    u32_le voice_channel_resource{};
    u32_le effect{};
    u32_le mixer{};
    u32_le sink{};
    u32_le performance{};
    u32_le splitter{};
    u32_le render_info{};
    INSERT_PADDING_WORDS(4);
};
static_assert(sizeof(UpdateDataSizes) == 0x38, "UpdateDataSizes is an invalid size");

struct UpdateDataHeader {
    u32_le revision{};
    UpdateDataSizes size{};
    u32_le total_size{};
};
static_assert(sizeof(UpdateDataHeader) == 0x40, "UpdateDataHeader is an invalid size");

struct AudioRendererParameter {
    u32_le sample_rate;
    u32_le sample_count;
    u32_le mix_buffer_count;
    u32_le submix_count;
    u32_le voice_count;
    u32_le sink_count;
    u32_le effect_count;
    u32_le performance_frame_count;
    u8 is_voice_drop_enabled;
    u8 unknown_21;
    u8 unknown_22;
    u8 execution_mode;
    u32_le splitter_count;
    u32_le num_splitter_send_channels;
    u32_le unknown_30;
    u32_le revision;
};
static_assert(sizeof(AudioRendererParameter) == 52, "AudioRendererParameter is an invalid size");

} // namespace AudioCommon
