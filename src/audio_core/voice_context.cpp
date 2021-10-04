// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include "audio_core/behavior_info.h"
#include "audio_core/voice_context.h"
#include "core/memory.h"

namespace AudioCore {

ServerVoiceChannelResource::ServerVoiceChannelResource(s32 id_) : id(id_) {}
ServerVoiceChannelResource::~ServerVoiceChannelResource() = default;

bool ServerVoiceChannelResource::InUse() const {
    return in_use;
}

float ServerVoiceChannelResource::GetCurrentMixVolumeAt(std::size_t i) const {
    ASSERT(i < AudioCommon::MAX_MIX_BUFFERS);
    return mix_volume.at(i);
}

float ServerVoiceChannelResource::GetLastMixVolumeAt(std::size_t i) const {
    ASSERT(i < AudioCommon::MAX_MIX_BUFFERS);
    return last_mix_volume.at(i);
}

void ServerVoiceChannelResource::Update(VoiceChannelResource::InParams& in_params) {
    in_use = in_params.in_use;
    // Update our mix volumes only if it's in use
    if (in_params.in_use) {
        mix_volume = in_params.mix_volume;
    }
}

void ServerVoiceChannelResource::UpdateLastMixVolumes() {
    last_mix_volume = mix_volume;
}

const std::array<float, AudioCommon::MAX_MIX_BUFFERS>&
ServerVoiceChannelResource::GetCurrentMixVolume() const {
    return mix_volume;
}

const std::array<float, AudioCommon::MAX_MIX_BUFFERS>&
ServerVoiceChannelResource::GetLastMixVolume() const {
    return last_mix_volume;
}

ServerVoiceInfo::ServerVoiceInfo() {
    Initialize();
}
ServerVoiceInfo::~ServerVoiceInfo() = default;

void ServerVoiceInfo::Initialize() {
    in_params.in_use = false;
    in_params.node_id = 0;
    in_params.id = 0;
    in_params.current_playstate = ServerPlayState::Stop;
    in_params.priority = 255;
    in_params.sample_rate = 0;
    in_params.sample_format = SampleFormat::Invalid;
    in_params.channel_count = 0;
    in_params.pitch = 0.0f;
    in_params.volume = 0.0f;
    in_params.last_volume = 0.0f;
    in_params.biquad_filter.fill({});
    in_params.wave_buffer_count = 0;
    in_params.wave_buffer_head = 0;
    in_params.mix_id = AudioCommon::NO_MIX;
    in_params.splitter_info_id = AudioCommon::NO_SPLITTER;
    in_params.additional_params_address = 0;
    in_params.additional_params_size = 0;
    in_params.is_new = false;
    out_params.played_sample_count = 0;
    out_params.wave_buffer_consumed = 0;
    in_params.voice_drop_flag = false;
    in_params.buffer_mapped = true;
    in_params.wave_buffer_flush_request_count = 0;
    in_params.was_biquad_filter_enabled.fill(false);

    for (auto& wave_buffer : in_params.wave_buffer) {
        wave_buffer.start_sample_offset = 0;
        wave_buffer.end_sample_offset = 0;
        wave_buffer.is_looping = false;
        wave_buffer.end_of_stream = false;
        wave_buffer.buffer_address = 0;
        wave_buffer.buffer_size = 0;
        wave_buffer.context_address = 0;
        wave_buffer.context_size = 0;
        wave_buffer.sent_to_dsp = true;
    }

    stored_samples.clear();
}

void ServerVoiceInfo::UpdateParameters(const VoiceInfo::InParams& voice_in,
                                       BehaviorInfo& behavior_info) {
    in_params.in_use = voice_in.is_in_use;
    in_params.id = voice_in.id;
    in_params.node_id = voice_in.node_id;
    in_params.last_playstate = in_params.current_playstate;
    switch (voice_in.play_state) {
    case PlayState::Paused:
        in_params.current_playstate = ServerPlayState::Paused;
        break;
    case PlayState::Stopped:
        if (in_params.current_playstate != ServerPlayState::Stop) {
            in_params.current_playstate = ServerPlayState::RequestStop;
        }
        break;
    case PlayState::Started:
        in_params.current_playstate = ServerPlayState::Play;
        break;
    default:
        UNREACHABLE_MSG("Unknown playstate {}", voice_in.play_state);
        break;
    }

    in_params.priority = voice_in.priority;
    in_params.sorting_order = voice_in.sorting_order;
    in_params.sample_rate = voice_in.sample_rate;
    in_params.sample_format = voice_in.sample_format;
    in_params.channel_count = voice_in.channel_count;
    in_params.pitch = voice_in.pitch;
    in_params.volume = voice_in.volume;
    in_params.biquad_filter = voice_in.biquad_filter;
    in_params.wave_buffer_count = voice_in.wave_buffer_count;
    in_params.wave_buffer_head = voice_in.wave_buffer_head;
    if (behavior_info.IsFlushVoiceWaveBuffersSupported()) {
        const auto in_request_count = in_params.wave_buffer_flush_request_count;
        const auto voice_request_count = voice_in.wave_buffer_flush_request_count;
        in_params.wave_buffer_flush_request_count =
            static_cast<u8>(in_request_count + voice_request_count);
    }
    in_params.mix_id = voice_in.mix_id;
    if (behavior_info.IsSplitterSupported()) {
        in_params.splitter_info_id = voice_in.splitter_info_id;
    } else {
        in_params.splitter_info_id = AudioCommon::NO_SPLITTER;
    }

    std::memcpy(in_params.voice_channel_resource_id.data(),
                voice_in.voice_channel_resource_ids.data(),
                sizeof(s32) * in_params.voice_channel_resource_id.size());

    if (behavior_info.IsVoicePlayedSampleCountResetAtLoopPointSupported()) {
        in_params.behavior_flags.is_played_samples_reset_at_loop_point =
            voice_in.behavior_flags.is_played_samples_reset_at_loop_point;
    } else {
        in_params.behavior_flags.is_played_samples_reset_at_loop_point.Assign(0);
    }
    if (behavior_info.IsVoicePitchAndSrcSkippedSupported()) {
        in_params.behavior_flags.is_pitch_and_src_skipped =
            voice_in.behavior_flags.is_pitch_and_src_skipped;
    } else {
        in_params.behavior_flags.is_pitch_and_src_skipped.Assign(0);
    }

    if (voice_in.is_voice_drop_flag_clear_requested) {
        in_params.voice_drop_flag = false;
    }

    if (in_params.additional_params_address != voice_in.additional_params_address ||
        in_params.additional_params_size != voice_in.additional_params_size) {
        in_params.additional_params_address = voice_in.additional_params_address;
        in_params.additional_params_size = voice_in.additional_params_size;
        // TODO(ogniK): Reattach buffer, do we actually need to? Maybe just signal to the DSP that
        // our context is new
    }
}

void ServerVoiceInfo::UpdateWaveBuffers(
    const VoiceInfo::InParams& voice_in,
    std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& voice_states,
    BehaviorInfo& behavior_info) {
    if (voice_in.is_new) {
        // Initialize our wave buffers
        for (auto& wave_buffer : in_params.wave_buffer) {
            wave_buffer.start_sample_offset = 0;
            wave_buffer.end_sample_offset = 0;
            wave_buffer.is_looping = false;
            wave_buffer.end_of_stream = false;
            wave_buffer.buffer_address = 0;
            wave_buffer.buffer_size = 0;
            wave_buffer.context_address = 0;
            wave_buffer.context_size = 0;
            wave_buffer.loop_start_sample = 0;
            wave_buffer.loop_end_sample = 0;
            wave_buffer.sent_to_dsp = true;
        }

        // Mark all our wave buffers as invalid
        for (std::size_t channel = 0; channel < static_cast<std::size_t>(in_params.channel_count);
             channel++) {
            for (std::size_t i = 0; i < AudioCommon::MAX_WAVE_BUFFERS; ++i) {
                voice_states[channel]->is_wave_buffer_valid[i] = false;
            }
        }
    }

    // Update our wave buffers
    for (std::size_t i = 0; i < AudioCommon::MAX_WAVE_BUFFERS; i++) {
        // Assume that we have at least 1 channel voice state
        const auto have_valid_wave_buffer = voice_states[0]->is_wave_buffer_valid[i];

        UpdateWaveBuffer(in_params.wave_buffer[i], voice_in.wave_buffer[i], in_params.sample_format,
                         have_valid_wave_buffer, behavior_info);
    }
}

void ServerVoiceInfo::UpdateWaveBuffer(ServerWaveBuffer& out_wavebuffer,
                                       const WaveBuffer& in_wave_buffer, SampleFormat sample_format,
                                       bool is_buffer_valid,
                                       [[maybe_unused]] BehaviorInfo& behavior_info) {
    if (!is_buffer_valid && out_wavebuffer.sent_to_dsp && out_wavebuffer.buffer_address != 0) {
        out_wavebuffer.buffer_address = 0;
        out_wavebuffer.buffer_size = 0;
    }

    if (!in_wave_buffer.sent_to_server || !in_params.buffer_mapped) {
        // Validate sample offset sizings
        if (sample_format == SampleFormat::Pcm16) {
            const s64 buffer_size = static_cast<s64>(in_wave_buffer.buffer_size);
            const s64 start = sizeof(s16) * in_wave_buffer.start_sample_offset;
            const s64 end = sizeof(s16) * in_wave_buffer.end_sample_offset;
            if (0 > start || start > buffer_size || 0 > end || end > buffer_size) {
                // TODO(ogniK): Write error info
                LOG_ERROR(Audio,
                          "PCM16 wavebuffer has an invalid size. Buffer has size 0x{:08X}, but "
                          "offsets were "
                          "{:08X} - 0x{:08X}",
                          buffer_size, sizeof(s16) * in_wave_buffer.start_sample_offset,
                          sizeof(s16) * in_wave_buffer.end_sample_offset);
                return;
            }
        } else if (sample_format == SampleFormat::Adpcm) {
            const s64 buffer_size = static_cast<s64>(in_wave_buffer.buffer_size);
            const s64 start_frames = in_wave_buffer.start_sample_offset / 14;
            const s64 start_extra = in_wave_buffer.start_sample_offset % 14 == 0
                                        ? 0
                                        : (in_wave_buffer.start_sample_offset % 14) / 2 + 1 +
                                              (in_wave_buffer.start_sample_offset % 2);
            const s64 start = start_frames * 8 + start_extra;
            const s64 end_frames = in_wave_buffer.end_sample_offset / 14;
            const s64 end_extra = in_wave_buffer.end_sample_offset % 14 == 0
                                      ? 0
                                      : (in_wave_buffer.end_sample_offset % 14) / 2 + 1 +
                                            (in_wave_buffer.end_sample_offset % 2);
            const s64 end = end_frames * 8 + end_extra;
            if (in_wave_buffer.start_sample_offset < 0 || start > buffer_size ||
                in_wave_buffer.end_sample_offset < 0 || end > buffer_size) {
                LOG_ERROR(Audio,
                          "ADPMC wavebuffer has an invalid size. Buffer has size 0x{:08X}, but "
                          "offsets were "
                          "{:08X} - 0x{:08X}",
                          in_wave_buffer.buffer_size, start, end);
                return;
            }
        }
        // TODO(ogniK): ADPCM Size error

        out_wavebuffer.sent_to_dsp = false;
        out_wavebuffer.start_sample_offset = in_wave_buffer.start_sample_offset;
        out_wavebuffer.end_sample_offset = in_wave_buffer.end_sample_offset;
        out_wavebuffer.is_looping = in_wave_buffer.is_looping;
        out_wavebuffer.end_of_stream = in_wave_buffer.end_of_stream;

        out_wavebuffer.buffer_address = in_wave_buffer.buffer_address;
        out_wavebuffer.buffer_size = in_wave_buffer.buffer_size;
        out_wavebuffer.context_address = in_wave_buffer.context_address;
        out_wavebuffer.context_size = in_wave_buffer.context_size;
        out_wavebuffer.loop_start_sample = in_wave_buffer.loop_start_sample;
        out_wavebuffer.loop_end_sample = in_wave_buffer.loop_end_sample;
        in_params.buffer_mapped =
            in_wave_buffer.buffer_address != 0 && in_wave_buffer.buffer_size != 0;
        // TODO(ogniK): Pool mapper attachment
        // TODO(ogniK): IsAdpcmLoopContextBugFixed
        if (sample_format == SampleFormat::Adpcm && in_wave_buffer.context_address != 0 &&
            in_wave_buffer.context_size != 0 && behavior_info.IsAdpcmLoopContextBugFixed()) {
        } else {
            out_wavebuffer.context_address = 0;
            out_wavebuffer.context_size = 0;
        }
    }
}

void ServerVoiceInfo::WriteOutStatus(
    VoiceInfo::OutParams& voice_out, VoiceInfo::InParams& voice_in,
    std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& voice_states) {
    if (voice_in.is_new || in_params.is_new) {
        in_params.is_new = true;
        voice_out.wave_buffer_consumed = 0;
        voice_out.played_sample_count = 0;
        voice_out.voice_dropped = false;
    } else {
        const auto& state = voice_states[0];
        voice_out.wave_buffer_consumed = state->wave_buffer_consumed;
        voice_out.played_sample_count = state->played_sample_count;
        voice_out.voice_dropped = state->voice_dropped;
    }
}

const ServerVoiceInfo::InParams& ServerVoiceInfo::GetInParams() const {
    return in_params;
}

ServerVoiceInfo::InParams& ServerVoiceInfo::GetInParams() {
    return in_params;
}

const ServerVoiceInfo::OutParams& ServerVoiceInfo::GetOutParams() const {
    return out_params;
}

ServerVoiceInfo::OutParams& ServerVoiceInfo::GetOutParams() {
    return out_params;
}

bool ServerVoiceInfo::ShouldSkip() const {
    // TODO(ogniK): Handle unmapped wave buffers or parameters
    return !in_params.in_use || in_params.wave_buffer_count == 0 || !in_params.buffer_mapped ||
           in_params.voice_drop_flag;
}

bool ServerVoiceInfo::UpdateForCommandGeneration(VoiceContext& voice_context) {
    std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT> dsp_voice_states{};
    if (in_params.is_new) {
        ResetResources(voice_context);
        in_params.last_volume = in_params.volume;
        in_params.is_new = false;
    }

    const s32 channel_count = in_params.channel_count;
    for (s32 i = 0; i < channel_count; i++) {
        const auto channel_resource = in_params.voice_channel_resource_id[i];
        dsp_voice_states[i] =
            &voice_context.GetDspSharedState(static_cast<std::size_t>(channel_resource));
    }
    return UpdateParametersForCommandGeneration(dsp_voice_states);
}

void ServerVoiceInfo::ResetResources(VoiceContext& voice_context) {
    const s32 channel_count = in_params.channel_count;
    for (s32 i = 0; i < channel_count; i++) {
        const auto channel_resource = in_params.voice_channel_resource_id[i];
        auto& dsp_state =
            voice_context.GetDspSharedState(static_cast<std::size_t>(channel_resource));
        dsp_state = {};
        voice_context.GetChannelResource(static_cast<std::size_t>(channel_resource))
            .UpdateLastMixVolumes();
    }
}

bool ServerVoiceInfo::UpdateParametersForCommandGeneration(
    std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& dsp_voice_states) {
    const s32 channel_count = in_params.channel_count;
    if (in_params.wave_buffer_flush_request_count > 0) {
        FlushWaveBuffers(in_params.wave_buffer_flush_request_count, dsp_voice_states,
                         channel_count);
        in_params.wave_buffer_flush_request_count = 0;
    }

    switch (in_params.current_playstate) {
    case ServerPlayState::Play: {
        for (std::size_t i = 0; i < AudioCommon::MAX_WAVE_BUFFERS; i++) {
            if (!in_params.wave_buffer[i].sent_to_dsp) {
                for (s32 channel = 0; channel < channel_count; channel++) {
                    dsp_voice_states[channel]->is_wave_buffer_valid[i] = true;
                }
                in_params.wave_buffer[i].sent_to_dsp = true;
            }
        }
        in_params.should_depop = false;
        return HasValidWaveBuffer(dsp_voice_states[0]);
    }
    case ServerPlayState::Paused:
    case ServerPlayState::Stop: {
        in_params.should_depop = in_params.last_playstate == ServerPlayState::Play;
        return in_params.should_depop;
    }
    case ServerPlayState::RequestStop: {
        for (std::size_t i = 0; i < AudioCommon::MAX_WAVE_BUFFERS; i++) {
            in_params.wave_buffer[i].sent_to_dsp = true;
            for (s32 channel = 0; channel < channel_count; channel++) {
                auto* dsp_state = dsp_voice_states[channel];

                if (dsp_state->is_wave_buffer_valid[i]) {
                    dsp_state->wave_buffer_index =
                        (dsp_state->wave_buffer_index + 1) % AudioCommon::MAX_WAVE_BUFFERS;
                    dsp_state->wave_buffer_consumed++;
                }

                dsp_state->is_wave_buffer_valid[i] = false;
            }
        }

        for (s32 channel = 0; channel < channel_count; channel++) {
            auto* dsp_state = dsp_voice_states[channel];
            dsp_state->offset = 0;
            dsp_state->played_sample_count = 0;
            dsp_state->fraction = 0;
            dsp_state->sample_history.fill(0);
            dsp_state->context = {};
        }

        in_params.current_playstate = ServerPlayState::Stop;
        in_params.should_depop = in_params.last_playstate == ServerPlayState::Play;
        return in_params.should_depop;
    }
    default:
        UNREACHABLE_MSG("Invalid playstate {}", in_params.current_playstate);
    }

    return false;
}

void ServerVoiceInfo::FlushWaveBuffers(
    u8 flush_count, std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& dsp_voice_states,
    s32 channel_count) {
    auto wave_head = in_params.wave_buffer_head;

    for (u8 i = 0; i < flush_count; i++) {
        in_params.wave_buffer[wave_head].sent_to_dsp = true;
        for (s32 channel = 0; channel < channel_count; channel++) {
            auto* dsp_state = dsp_voice_states[channel];
            dsp_state->wave_buffer_consumed++;
            dsp_state->is_wave_buffer_valid[wave_head] = false;
            dsp_state->wave_buffer_index =
                (dsp_state->wave_buffer_index + 1) % AudioCommon::MAX_WAVE_BUFFERS;
        }
        wave_head = (wave_head + 1) % AudioCommon::MAX_WAVE_BUFFERS;
    }
}

bool ServerVoiceInfo::HasValidWaveBuffer(const VoiceState* state) const {
    const auto& valid_wb = state->is_wave_buffer_valid;
    return std::find(valid_wb.begin(), valid_wb.end(), true) != valid_wb.end();
}

void ServerVoiceInfo::SetWaveBufferCompleted(VoiceState& dsp_state,
                                             const ServerWaveBuffer& wave_buffer) {
    dsp_state.is_wave_buffer_valid[dsp_state.wave_buffer_index] = false;
    dsp_state.wave_buffer_consumed++;
    dsp_state.wave_buffer_index = (dsp_state.wave_buffer_index + 1) % AudioCommon::MAX_WAVE_BUFFERS;
    dsp_state.loop_count = 0;
    if (wave_buffer.end_of_stream) {
        dsp_state.played_sample_count = 0;
    }
}

VoiceContext::VoiceContext(std::size_t voice_count_) : voice_count{voice_count_} {
    for (std::size_t i = 0; i < voice_count; i++) {
        voice_channel_resources.emplace_back(static_cast<s32>(i));
        sorted_voice_info.push_back(&voice_info.emplace_back());
        voice_states.emplace_back();
        dsp_voice_states.emplace_back();
    }
}

VoiceContext::~VoiceContext() {
    sorted_voice_info.clear();
}

std::size_t VoiceContext::GetVoiceCount() const {
    return voice_count;
}

ServerVoiceChannelResource& VoiceContext::GetChannelResource(std::size_t i) {
    ASSERT(i < voice_count);
    return voice_channel_resources.at(i);
}

const ServerVoiceChannelResource& VoiceContext::GetChannelResource(std::size_t i) const {
    ASSERT(i < voice_count);
    return voice_channel_resources.at(i);
}

VoiceState& VoiceContext::GetState(std::size_t i) {
    ASSERT(i < voice_count);
    return voice_states.at(i);
}

const VoiceState& VoiceContext::GetState(std::size_t i) const {
    ASSERT(i < voice_count);
    return voice_states.at(i);
}

VoiceState& VoiceContext::GetDspSharedState(std::size_t i) {
    ASSERT(i < voice_count);
    return dsp_voice_states.at(i);
}

const VoiceState& VoiceContext::GetDspSharedState(std::size_t i) const {
    ASSERT(i < voice_count);
    return dsp_voice_states.at(i);
}

ServerVoiceInfo& VoiceContext::GetInfo(std::size_t i) {
    ASSERT(i < voice_count);
    return voice_info.at(i);
}

const ServerVoiceInfo& VoiceContext::GetInfo(std::size_t i) const {
    ASSERT(i < voice_count);
    return voice_info.at(i);
}

ServerVoiceInfo& VoiceContext::GetSortedInfo(std::size_t i) {
    ASSERT(i < voice_count);
    return *sorted_voice_info.at(i);
}

const ServerVoiceInfo& VoiceContext::GetSortedInfo(std::size_t i) const {
    ASSERT(i < voice_count);
    return *sorted_voice_info.at(i);
}

s32 VoiceContext::DecodePcm16(s32* output_buffer, ServerWaveBuffer* wave_buffer, s32 channel,
                              s32 channel_count, s32 buffer_offset, s32 sample_count,
                              Core::Memory::Memory& memory) {
    if (wave_buffer->buffer_address == 0) {
        return 0;
    }
    if (wave_buffer->buffer_size == 0) {
        return 0;
    }
    if (wave_buffer->end_sample_offset < wave_buffer->start_sample_offset) {
        return 0;
    }

    const auto samples_remaining =
        (wave_buffer->end_sample_offset - wave_buffer->start_sample_offset) - buffer_offset;
    const auto start_offset = (wave_buffer->start_sample_offset + buffer_offset) * channel_count;
    const auto buffer_pos = wave_buffer->buffer_address + start_offset;

    s16* buffer_data = reinterpret_cast<s16*>(memory.GetPointer(buffer_pos));

    const auto samples_processed = std::min(sample_count, samples_remaining);

    // Fast path
    if (channel_count == 1) {
        for (std::ptrdiff_t i = 0; i < samples_processed; i++) {
            output_buffer[i] = buffer_data[i];
        }
    } else {
        for (std::ptrdiff_t i = 0; i < samples_processed; i++) {
            output_buffer[i] = buffer_data[i * channel_count + channel];
        }
    }

    return samples_processed;
}

void VoiceContext::SortInfo() {
    for (std::size_t i = 0; i < voice_count; i++) {
        sorted_voice_info[i] = &voice_info[i];
    }

    std::sort(sorted_voice_info.begin(), sorted_voice_info.end(),
              [](const ServerVoiceInfo* lhs, const ServerVoiceInfo* rhs) {
                  const auto& lhs_in = lhs->GetInParams();
                  const auto& rhs_in = rhs->GetInParams();
                  // Sort by priority
                  if (lhs_in.priority != rhs_in.priority) {
                      return lhs_in.priority > rhs_in.priority;
                  } else {
                      // If the priorities match, sort by sorting order
                      return lhs_in.sorting_order > rhs_in.sorting_order;
                  }
              });
}

void VoiceContext::UpdateStateByDspShared() {
    voice_states = dsp_voice_states;
}

} // namespace AudioCore
