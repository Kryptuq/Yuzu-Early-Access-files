// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/behavior_info.h"
#include "audio_core/effect_context.h"
#include "audio_core/info_updater.h"
#include "audio_core/memory_pool.h"
#include "audio_core/mix_context.h"
#include "audio_core/sink_context.h"
#include "audio_core/splitter_context.h"
#include "audio_core/voice_context.h"
#include "common/logging/log.h"

namespace AudioCore {

InfoUpdater::InfoUpdater(const std::vector<u8>& in_params_, std::vector<u8>& out_params_,
                         BehaviorInfo& behavior_info_)
    : in_params(in_params_), out_params(out_params_), behavior_info(behavior_info_) {
    ASSERT(
        AudioCommon::CanConsumeBuffer(in_params.size(), 0, sizeof(AudioCommon::UpdateDataHeader)));
    std::memcpy(&input_header, in_params.data(), sizeof(AudioCommon::UpdateDataHeader));
    output_header.total_size = sizeof(AudioCommon::UpdateDataHeader);
}

InfoUpdater::~InfoUpdater() = default;

bool InfoUpdater::UpdateBehaviorInfo(BehaviorInfo& in_behavior_info) {
    if (input_header.size.behavior != sizeof(BehaviorInfo::InParams)) {
        LOG_ERROR(Audio, "Behavior info is an invalid size, expecting 0x{:X} but got 0x{:X}",
                  sizeof(BehaviorInfo::InParams), input_header.size.behavior);
        return false;
    }

    if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset,
                                       sizeof(BehaviorInfo::InParams))) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    BehaviorInfo::InParams behavior_in{};
    std::memcpy(&behavior_in, in_params.data() + input_offset, sizeof(BehaviorInfo::InParams));
    input_offset += sizeof(BehaviorInfo::InParams);

    // Make sure it's an audio revision we can actually support
    if (!AudioCommon::IsValidRevision(behavior_in.revision)) {
        LOG_ERROR(Audio, "Invalid input revision, revision=0x{:08X}", behavior_in.revision);
        return false;
    }

    // Make sure that our behavior info revision matches the input
    if (in_behavior_info.GetUserRevision() != behavior_in.revision) {
        LOG_ERROR(Audio,
                  "User revision differs from input revision, expecting 0x{:08X} but got 0x{:08X}",
                  in_behavior_info.GetUserRevision(), behavior_in.revision);
        return false;
    }

    // Update behavior info flags
    in_behavior_info.ClearError();
    in_behavior_info.UpdateFlags(behavior_in.flags);

    return true;
}

bool InfoUpdater::UpdateMemoryPools(std::vector<ServerMemoryPoolInfo>& memory_pool_info) {
    const auto memory_pool_count = memory_pool_info.size();
    const auto total_memory_pool_in = sizeof(ServerMemoryPoolInfo::InParams) * memory_pool_count;
    const auto total_memory_pool_out = sizeof(ServerMemoryPoolInfo::OutParams) * memory_pool_count;

    if (input_header.size.memory_pool != total_memory_pool_in) {
        LOG_ERROR(Audio, "Memory pools are an invalid size, expecting 0x{:X} but got 0x{:X}",
                  total_memory_pool_in, input_header.size.memory_pool);
        return false;
    }

    if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset, total_memory_pool_in)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    std::vector<ServerMemoryPoolInfo::InParams> mempool_in(memory_pool_count);
    std::vector<ServerMemoryPoolInfo::OutParams> mempool_out(memory_pool_count);

    std::memcpy(mempool_in.data(), in_params.data() + input_offset, total_memory_pool_in);
    input_offset += total_memory_pool_in;

    // Update our memory pools
    for (std::size_t i = 0; i < memory_pool_count; i++) {
        if (!memory_pool_info[i].Update(mempool_in[i], mempool_out[i])) {
            LOG_ERROR(Audio, "Failed to update memory pool {}!", i);
            return false;
        }
    }

    if (!AudioCommon::CanConsumeBuffer(out_params.size(), output_offset,
                                       sizeof(BehaviorInfo::InParams))) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    std::memcpy(out_params.data() + output_offset, mempool_out.data(), total_memory_pool_out);
    output_offset += total_memory_pool_out;
    output_header.size.memory_pool = static_cast<u32>(total_memory_pool_out);
    return true;
}

bool InfoUpdater::UpdateVoiceChannelResources(VoiceContext& voice_context) {
    const auto voice_count = voice_context.GetVoiceCount();
    const auto voice_size = voice_count * sizeof(VoiceChannelResource::InParams);
    std::vector<VoiceChannelResource::InParams> resources_in(voice_count);

    if (input_header.size.voice_channel_resource != voice_size) {
        LOG_ERROR(Audio, "VoiceChannelResource is an invalid size, expecting 0x{:X} but got 0x{:X}",
                  voice_size, input_header.size.voice_channel_resource);
        return false;
    }

    if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset, voice_size)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    std::memcpy(resources_in.data(), in_params.data() + input_offset, voice_size);
    input_offset += voice_size;

    // Update our channel resources
    for (std::size_t i = 0; i < voice_count; i++) {
        // Grab our channel resource
        auto& resource = voice_context.GetChannelResource(i);
        resource.Update(resources_in[i]);
    }

    return true;
}

bool InfoUpdater::UpdateVoices(VoiceContext& voice_context,
                               [[maybe_unused]] std::vector<ServerMemoryPoolInfo>& memory_pool_info,
                               [[maybe_unused]] VAddr audio_codec_dsp_addr) {
    const auto voice_count = voice_context.GetVoiceCount();
    std::vector<VoiceInfo::InParams> voice_in(voice_count);
    std::vector<VoiceInfo::OutParams> voice_out(voice_count);

    const auto voice_in_size = voice_count * sizeof(VoiceInfo::InParams);
    const auto voice_out_size = voice_count * sizeof(VoiceInfo::OutParams);

    if (input_header.size.voice != voice_in_size) {
        LOG_ERROR(Audio, "Voices are an invalid size, expecting 0x{:X} but got 0x{:X}",
                  voice_in_size, input_header.size.voice);
        return false;
    }

    if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset, voice_in_size)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    std::memcpy(voice_in.data(), in_params.data() + input_offset, voice_in_size);
    input_offset += voice_in_size;

    // Set all voices to not be in use
    for (std::size_t i = 0; i < voice_count; i++) {
        voice_context.GetInfo(i).GetInParams().in_use = false;
    }

    // Update our voices
    for (std::size_t i = 0; i < voice_count; i++) {
        auto& voice_in_params = voice_in[i];
        const auto channel_count = static_cast<std::size_t>(voice_in_params.channel_count);
        // Skip if it's not currently in use
        if (!voice_in_params.is_in_use) {
            continue;
        }
        // Voice states for each channel
        std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT> voice_states{};
        ASSERT(static_cast<std::size_t>(voice_in_params.id) < voice_count);

        // Grab our current voice info
        auto& voice_info = voice_context.GetInfo(static_cast<std::size_t>(voice_in_params.id));

        ASSERT(channel_count <= AudioCommon::MAX_CHANNEL_COUNT);

        // Get all our channel voice states
        for (std::size_t channel = 0; channel < channel_count; channel++) {
            voice_states[channel] =
                &voice_context.GetState(voice_in_params.voice_channel_resource_ids[channel]);
        }

        if (voice_in_params.is_new) {
            // Default our values for our voice
            voice_info.Initialize();

            // Zero out our voice states
            for (std::size_t channel = 0; channel < channel_count; channel++) {
                std::memset(voice_states[channel], 0, sizeof(VoiceState));
            }
        }

        // Update our voice
        voice_info.UpdateParameters(voice_in_params, behavior_info);
        // TODO(ogniK): Handle mapping errors with behavior info based on in params response

        // Update our wave buffers
        voice_info.UpdateWaveBuffers(voice_in_params, voice_states, behavior_info);
        voice_info.WriteOutStatus(voice_out[i], voice_in_params, voice_states);
    }

    if (!AudioCommon::CanConsumeBuffer(out_params.size(), output_offset, voice_out_size)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }
    std::memcpy(out_params.data() + output_offset, voice_out.data(), voice_out_size);
    output_offset += voice_out_size;
    output_header.size.voice = static_cast<u32>(voice_out_size);
    return true;
}

bool InfoUpdater::UpdateEffects(EffectContext& effect_context, bool is_active) {
    const auto effect_count = effect_context.GetCount();
    std::vector<EffectInfo::InParams> effect_in(effect_count);
    std::vector<EffectInfo::OutParams> effect_out(effect_count);

    const auto total_effect_in = effect_count * sizeof(EffectInfo::InParams);
    const auto total_effect_out = effect_count * sizeof(EffectInfo::OutParams);

    if (input_header.size.effect != total_effect_in) {
        LOG_ERROR(Audio, "Effects are an invalid size, expecting 0x{:X} but got 0x{:X}",
                  total_effect_in, input_header.size.effect);
        return false;
    }

    if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset, total_effect_in)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    std::memcpy(effect_in.data(), in_params.data() + input_offset, total_effect_in);
    input_offset += total_effect_in;

    // Update effects
    for (std::size_t i = 0; i < effect_count; i++) {
        auto* info = effect_context.GetInfo(i);
        if (effect_in[i].type != info->GetType()) {
            info = effect_context.RetargetEffect(i, effect_in[i].type);
        }

        info->Update(effect_in[i]);

        if ((!is_active && info->GetUsage() != UsageState::Initialized) ||
            info->GetUsage() == UsageState::Stopped) {
            effect_out[i].status = UsageStatus::Removed;
        } else {
            effect_out[i].status = UsageStatus::Used;
        }
    }

    if (!AudioCommon::CanConsumeBuffer(out_params.size(), output_offset, total_effect_out)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    std::memcpy(out_params.data() + output_offset, effect_out.data(), total_effect_out);
    output_offset += total_effect_out;
    output_header.size.effect = static_cast<u32>(total_effect_out);

    return true;
}

bool InfoUpdater::UpdateSplitterInfo(SplitterContext& splitter_context) {
    std::size_t start_offset = input_offset;
    std::size_t bytes_read{};
    // Update splitter context
    if (!splitter_context.Update(in_params, input_offset, bytes_read)) {
        LOG_ERROR(Audio, "Failed to update splitter context!");
        return false;
    }

    const auto consumed = input_offset - start_offset;

    if (input_header.size.splitter != consumed) {
        LOG_ERROR(Audio, "Splitters is an invalid size, expecting 0x{:X} but got 0x{:X}",
                  bytes_read, input_header.size.splitter);
        return false;
    }

    return true;
}

Result InfoUpdater::UpdateMixes(MixContext& mix_context, std::size_t mix_buffer_count,
                                SplitterContext& splitter_context, EffectContext& effect_context) {
    std::vector<MixInfo::InParams> mix_in_params;

    if (!behavior_info.IsMixInParameterDirtyOnlyUpdateSupported()) {
        // If we're not dirty, get ALL mix in parameters
        const auto context_mix_count = mix_context.GetCount();
        const auto total_mix_in = context_mix_count * sizeof(MixInfo::InParams);
        if (input_header.size.mixer != total_mix_in) {
            LOG_ERROR(Audio, "Mixer is an invalid size, expecting 0x{:X} but got 0x{:X}",
                      total_mix_in, input_header.size.mixer);
            return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
        }

        if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset, total_mix_in)) {
            LOG_ERROR(Audio, "Buffer is an invalid size!");
            return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
        }

        mix_in_params.resize(context_mix_count);
        std::memcpy(mix_in_params.data(), in_params.data() + input_offset, total_mix_in);

        input_offset += total_mix_in;
    } else {
        // Only update the "dirty" mixes
        MixInfo::DirtyHeader dirty_header{};
        if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset,
                                           sizeof(MixInfo::DirtyHeader))) {
            LOG_ERROR(Audio, "Buffer is an invalid size!");
            return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
        }

        std::memcpy(&dirty_header, in_params.data() + input_offset, sizeof(MixInfo::DirtyHeader));
        input_offset += sizeof(MixInfo::DirtyHeader);

        const auto total_mix_in =
            dirty_header.mixer_count * sizeof(MixInfo::InParams) + sizeof(MixInfo::DirtyHeader);

        if (input_header.size.mixer != total_mix_in) {
            LOG_ERROR(Audio, "Mixer is an invalid size, expecting 0x{:X} but got 0x{:X}",
                      total_mix_in, input_header.size.mixer);
            return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
        }

        if (dirty_header.mixer_count != 0) {
            mix_in_params.resize(dirty_header.mixer_count);
            std::memcpy(mix_in_params.data(), in_params.data() + input_offset,
                        mix_in_params.size() * sizeof(MixInfo::InParams));
            input_offset += mix_in_params.size() * sizeof(MixInfo::InParams);
        }
    }

    // Get our total input count
    const auto mix_count = mix_in_params.size();

    if (!behavior_info.IsMixInParameterDirtyOnlyUpdateSupported()) {
        // Only verify our buffer count if we're not dirty
        std::size_t total_buffer_count{};
        for (std::size_t i = 0; i < mix_count; i++) {
            const auto& in = mix_in_params[i];
            total_buffer_count += in.buffer_count;
            if (static_cast<std::size_t>(in.dest_mix_id) > mix_count &&
                in.dest_mix_id != AudioCommon::NO_MIX && in.mix_id != AudioCommon::FINAL_MIX) {
                LOG_ERROR(
                    Audio,
                    "Invalid mix destination, mix_id={:X}, dest_mix_id={:X}, mix_buffer_count={:X}",
                    in.mix_id, in.dest_mix_id, mix_buffer_count);
                return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
            }
        }

        if (total_buffer_count > mix_buffer_count) {
            LOG_ERROR(Audio,
                      "Too many mix buffers used! mix_buffer_count={:X}, requesting_buffers={:X}",
                      mix_buffer_count, total_buffer_count);
            return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
        }
    }

    if (mix_buffer_count == 0) {
        LOG_ERROR(Audio, "No mix buffers!");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    bool should_sort = false;
    for (std::size_t i = 0; i < mix_count; i++) {
        const auto& mix_in = mix_in_params[i];
        std::size_t target_mix{};
        if (behavior_info.IsMixInParameterDirtyOnlyUpdateSupported()) {
            target_mix = mix_in.mix_id;
        } else {
            // Non dirty supported games just use i instead of the actual mix_id
            target_mix = i;
        }
        auto& mix_info = mix_context.GetInfo(target_mix);
        auto& mix_info_params = mix_info.GetInParams();
        if (mix_info_params.in_use != mix_in.in_use) {
            mix_info_params.in_use = mix_in.in_use;
            mix_info.ResetEffectProcessingOrder();
            should_sort = true;
        }

        if (mix_in.in_use) {
            should_sort |= mix_info.Update(mix_context.GetEdgeMatrix(), mix_in, behavior_info,
                                           splitter_context, effect_context);
        }
    }

    if (should_sort && behavior_info.IsSplitterSupported()) {
        // Sort our splitter data
        if (!mix_context.TsortInfo(splitter_context)) {
            return AudioCommon::Audren::ERR_SPLITTER_SORT_FAILED;
        }
    }

    // TODO(ogniK): Sort when splitter is suppoorted

    return ResultSuccess;
}

bool InfoUpdater::UpdateSinks(SinkContext& sink_context) {
    const auto sink_count = sink_context.GetCount();
    std::vector<SinkInfo::InParams> sink_in_params(sink_count);
    const auto total_sink_in = sink_count * sizeof(SinkInfo::InParams);

    if (input_header.size.sink != total_sink_in) {
        LOG_ERROR(Audio, "Sinks are an invalid size, expecting 0x{:X} but got 0x{:X}",
                  total_sink_in, input_header.size.effect);
        return false;
    }

    if (!AudioCommon::CanConsumeBuffer(in_params.size(), input_offset, total_sink_in)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    std::memcpy(sink_in_params.data(), in_params.data() + input_offset, total_sink_in);
    input_offset += total_sink_in;

    // TODO(ogniK): Properly update sinks
    if (!sink_in_params.empty()) {
        sink_context.UpdateMainSink(sink_in_params[0]);
    }

    output_header.size.sink = static_cast<u32>(0x20 * sink_count);
    output_offset += 0x20 * sink_count;
    return true;
}

bool InfoUpdater::UpdatePerformanceBuffer() {
    output_header.size.performance = 0x10;
    output_offset += 0x10;
    return true;
}

bool InfoUpdater::UpdateErrorInfo([[maybe_unused]] BehaviorInfo& in_behavior_info) {
    const auto total_beahvior_info_out = sizeof(BehaviorInfo::OutParams);

    if (!AudioCommon::CanConsumeBuffer(out_params.size(), output_offset, total_beahvior_info_out)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    BehaviorInfo::OutParams behavior_info_out{};
    behavior_info.CopyErrorInfo(behavior_info_out);

    std::memcpy(out_params.data() + output_offset, &behavior_info_out, total_beahvior_info_out);
    output_offset += total_beahvior_info_out;
    output_header.size.behavior = total_beahvior_info_out;

    return true;
}

struct RendererInfo {
    u64_le elasped_frame_count{};
    INSERT_PADDING_WORDS(2);
};
static_assert(sizeof(RendererInfo) == 0x10, "RendererInfo is an invalid size");

bool InfoUpdater::UpdateRendererInfo(std::size_t elapsed_frame_count) {
    const auto total_renderer_info_out = sizeof(RendererInfo);
    if (!AudioCommon::CanConsumeBuffer(out_params.size(), output_offset, total_renderer_info_out)) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }
    RendererInfo out{};
    out.elasped_frame_count = elapsed_frame_count;
    std::memcpy(out_params.data() + output_offset, &out, total_renderer_info_out);
    output_offset += total_renderer_info_out;
    output_header.size.render_info = total_renderer_info_out;

    return true;
}

bool InfoUpdater::CheckConsumedSize() const {
    if (output_offset != out_params.size()) {
        LOG_ERROR(Audio, "Output is not consumed! Consumed {}, but requires {}. {} bytes remaining",
                  output_offset, out_params.size(), out_params.size() - output_offset);
        return false;
    }
    /*if (input_offset != in_params.size()) {
        LOG_ERROR(Audio, "Input is not consumed!");
        return false;
    }*/
    return true;
}

bool InfoUpdater::WriteOutputHeader() {
    if (!AudioCommon::CanConsumeBuffer(out_params.size(), 0,
                                       sizeof(AudioCommon::UpdateDataHeader))) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }
    output_header.revision = AudioCommon::CURRENT_PROCESS_REVISION;
    const auto& sz = output_header.size;
    output_header.total_size += sz.behavior + sz.memory_pool + sz.voice +
                                sz.voice_channel_resource + sz.effect + sz.mixer + sz.sink +
                                sz.performance + sz.splitter + sz.render_info;

    std::memcpy(out_params.data(), &output_header, sizeof(AudioCommon::UpdateDataHeader));
    return true;
}

} // namespace AudioCore
