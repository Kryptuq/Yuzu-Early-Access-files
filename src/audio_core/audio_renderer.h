// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "audio_core/behavior_info.h"
#include "audio_core/command_generator.h"
#include "audio_core/common.h"
#include "audio_core/effect_context.h"
#include "audio_core/memory_pool.h"
#include "audio_core/mix_context.h"
#include "audio_core/sink_context.h"
#include "audio_core/splitter_context.h"
#include "audio_core/stream.h"
#include "audio_core/voice_context.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/result.h"

namespace Core::Timing {
class CoreTiming;
}

namespace Core::Memory {
class Memory;
}

namespace AudioCore {
using DSPStateHolder = std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>;

class AudioOut;

class AudioRenderer {
public:
    AudioRenderer(Core::Timing::CoreTiming& core_timing, Core::Memory::Memory& memory_,
                  AudioCommon::AudioRendererParameter params,
                  Stream::ReleaseCallback&& release_callback, std::size_t instance_number);
    ~AudioRenderer();

    [[nodiscard]] ResultCode UpdateAudioRenderer(const std::vector<u8>& input_params,
                                                 std::vector<u8>& output_params);
    void QueueMixedBuffer(Buffer::Tag tag);
    void ReleaseAndQueueBuffers();
    [[nodiscard]] u32 GetSampleRate() const;
    [[nodiscard]] u32 GetSampleCount() const;
    [[nodiscard]] u32 GetMixBufferCount() const;
    [[nodiscard]] Stream::State GetStreamState() const;

private:
    BehaviorInfo behavior_info{};

    AudioCommon::AudioRendererParameter worker_params;
    std::vector<ServerMemoryPoolInfo> memory_pool_info;
    VoiceContext voice_context;
    EffectContext effect_context;
    MixContext mix_context;
    SinkContext sink_context;
    SplitterContext splitter_context;
    std::vector<VoiceState> voices;
    std::unique_ptr<AudioOut> audio_out;
    StreamPtr stream;
    Core::Memory::Memory& memory;
    CommandGenerator command_generator;
    std::size_t elapsed_frame_count{};
};

} // namespace AudioCore
