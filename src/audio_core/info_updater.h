// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "audio_core/common.h"
#include "common/common_types.h"

namespace AudioCore {

class BehaviorInfo;
class ServerMemoryPoolInfo;
class VoiceContext;
class EffectContext;
class MixContext;
class SinkContext;
class SplitterContext;

class InfoUpdater {
public:
    // TODO(ogniK): Pass process handle when we support it
    InfoUpdater(const std::vector<u8>& in_params_, std::vector<u8>& out_params_,
                BehaviorInfo& behavior_info_);
    ~InfoUpdater();

    bool UpdateBehaviorInfo(BehaviorInfo& in_behavior_info);
    bool UpdateMemoryPools(std::vector<ServerMemoryPoolInfo>& memory_pool_info);
    bool UpdateVoiceChannelResources(VoiceContext& voice_context);
    bool UpdateVoices(VoiceContext& voice_context,
                      std::vector<ServerMemoryPoolInfo>& memory_pool_info,
                      VAddr audio_codec_dsp_addr);
    bool UpdateEffects(EffectContext& effect_context, bool is_active);
    bool UpdateSplitterInfo(SplitterContext& splitter_context);
    Result UpdateMixes(MixContext& mix_context, std::size_t mix_buffer_count,
                       SplitterContext& splitter_context, EffectContext& effect_context);
    bool UpdateSinks(SinkContext& sink_context);
    bool UpdatePerformanceBuffer();
    bool UpdateErrorInfo(BehaviorInfo& in_behavior_info);
    bool UpdateRendererInfo(std::size_t elapsed_frame_count);
    bool CheckConsumedSize() const;

    bool WriteOutputHeader();

private:
    const std::vector<u8>& in_params;
    std::vector<u8>& out_params;
    BehaviorInfo& behavior_info;

    AudioCommon::UpdateDataHeader input_header{};
    AudioCommon::UpdateDataHeader output_header{};

    std::size_t input_offset{sizeof(AudioCommon::UpdateDataHeader)};
    std::size_t output_offset{sizeof(AudioCommon::UpdateDataHeader)};
};

} // namespace AudioCore
