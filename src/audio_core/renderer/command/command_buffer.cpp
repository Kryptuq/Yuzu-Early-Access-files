// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/command/command_buffer.h"
#include "audio_core/renderer/command/command_list_header.h"
#include "audio_core/renderer/command/command_processing_time_estimator.h"
#include "audio_core/renderer/effect/biquad_filter.h"
#include "audio_core/renderer/effect/delay.h"
#include "audio_core/renderer/effect/reverb.h"
#include "audio_core/renderer/memory/memory_pool_info.h"
#include "audio_core/renderer/mix/mix_info.h"
#include "audio_core/renderer/sink/circular_buffer_sink_info.h"
#include "audio_core/renderer/sink/device_sink_info.h"
#include "audio_core/renderer/sink/sink_info_base.h"
#include "audio_core/renderer/voice/voice_info.h"
#include "audio_core/renderer/voice/voice_state.h"

namespace AudioCore::AudioRenderer {

template <typename T, CommandId Id>
T& CommandBuffer::GenerateStart(const s32 node_id) {
    if (size + sizeof(T) >= command_list.size_bytes()) {
        LOG_ERROR(
            Service_Audio,
            "Attempting to write commands beyond the end of allocated command buffer memory!");
        UNREACHABLE();
    }

    auto& cmd{*std::construct_at<T>(reinterpret_cast<T*>(&command_list[size]))};

    cmd.magic = CommandMagic;
    cmd.enabled = true;
    cmd.type = Id;
    cmd.size = sizeof(T);
    cmd.node_id = node_id;

    return cmd;
}

template <typename T>
void CommandBuffer::GenerateEnd(T& cmd) {
    cmd.estimated_process_time = time_estimator->Estimate(cmd);
    estimated_process_time += cmd.estimated_process_time;
    size += sizeof(T);
    count++;
}

void CommandBuffer::GeneratePcmInt16Version1Command(const s32 node_id,
                                                    const MemoryPoolInfo& memory_pool_,
                                                    VoiceInfo& voice_info,
                                                    const VoiceState& voice_state,
                                                    const s16 buffer_count, const s8 channel) {
    auto& cmd{
        GenerateStart<PcmInt16DataSourceVersion1Command, CommandId::DataSourcePcmInt16Version1>(
            node_id)};

    cmd.src_quality = voice_info.src_quality;
    cmd.output_index = buffer_count + channel;
    cmd.flags = voice_info.flags & 3;
    cmd.sample_rate = voice_info.sample_rate;
    cmd.pitch = voice_info.pitch;
    cmd.channel_index = channel;
    cmd.channel_count = voice_info.channel_count;

    for (u32 i = 0; i < MaxWaveBuffers; i++) {
        voice_info.wavebuffers[i].Copy(cmd.wave_buffers[i]);
    }

    cmd.voice_state = memory_pool_.Translate(CpuAddr(&voice_state), sizeof(VoiceState));

    GenerateEnd<PcmInt16DataSourceVersion1Command>(cmd);
}

void CommandBuffer::GeneratePcmInt16Version2Command(const s32 node_id, VoiceInfo& voice_info,
                                                    const VoiceState& voice_state,
                                                    const s16 buffer_count, const s8 channel) {
    auto& cmd{
        GenerateStart<PcmInt16DataSourceVersion2Command, CommandId::DataSourcePcmInt16Version2>(
            node_id)};

    cmd.src_quality = voice_info.src_quality;
    cmd.output_index = buffer_count + channel;
    cmd.flags = voice_info.flags & 3;
    cmd.sample_rate = voice_info.sample_rate;
    cmd.pitch = voice_info.pitch;
    cmd.channel_index = channel;
    cmd.channel_count = voice_info.channel_count;

    for (u32 i = 0; i < MaxWaveBuffers; i++) {
        voice_info.wavebuffers[i].Copy(cmd.wave_buffers[i]);
    }

    cmd.voice_state = memory_pool->Translate(CpuAddr(&voice_state), sizeof(VoiceState));

    GenerateEnd<PcmInt16DataSourceVersion2Command>(cmd);
}

void CommandBuffer::GeneratePcmFloatVersion1Command(const s32 node_id,
                                                    const MemoryPoolInfo& memory_pool_,
                                                    VoiceInfo& voice_info,
                                                    const VoiceState& voice_state,
                                                    const s16 buffer_count, const s8 channel) {
    auto& cmd{
        GenerateStart<PcmFloatDataSourceVersion1Command, CommandId::DataSourcePcmFloatVersion1>(
            node_id)};

    cmd.src_quality = voice_info.src_quality;
    cmd.output_index = buffer_count + channel;
    cmd.flags = voice_info.flags & 3;
    cmd.sample_rate = voice_info.sample_rate;
    cmd.pitch = voice_info.pitch;
    cmd.channel_index = channel;
    cmd.channel_count = voice_info.channel_count;

    for (u32 i = 0; i < MaxWaveBuffers; i++) {
        voice_info.wavebuffers[i].Copy(cmd.wave_buffers[i]);
    }

    cmd.voice_state = memory_pool_.Translate(CpuAddr(&voice_state), sizeof(VoiceState));

    GenerateEnd<PcmFloatDataSourceVersion1Command>(cmd);
}

void CommandBuffer::GeneratePcmFloatVersion2Command(const s32 node_id, VoiceInfo& voice_info,
                                                    const VoiceState& voice_state,
                                                    const s16 buffer_count, const s8 channel) {
    auto& cmd{
        GenerateStart<PcmFloatDataSourceVersion2Command, CommandId::DataSourcePcmFloatVersion2>(
            node_id)};

    cmd.src_quality = voice_info.src_quality;
    cmd.output_index = buffer_count + channel;
    cmd.flags = voice_info.flags & 3;
    cmd.sample_rate = voice_info.sample_rate;
    cmd.pitch = voice_info.pitch;
    cmd.channel_index = channel;
    cmd.channel_count = voice_info.channel_count;

    for (u32 i = 0; i < MaxWaveBuffers; i++) {
        voice_info.wavebuffers[i].Copy(cmd.wave_buffers[i]);
    }

    cmd.voice_state = memory_pool->Translate(CpuAddr(&voice_state), sizeof(VoiceState));

    GenerateEnd<PcmFloatDataSourceVersion2Command>(cmd);
}

void CommandBuffer::GenerateAdpcmVersion1Command(const s32 node_id,
                                                 const MemoryPoolInfo& memory_pool_,
                                                 VoiceInfo& voice_info,
                                                 const VoiceState& voice_state,
                                                 const s16 buffer_count, const s8 channel) {
    auto& cmd{
        GenerateStart<AdpcmDataSourceVersion1Command, CommandId::DataSourceAdpcmVersion1>(node_id)};

    cmd.src_quality = voice_info.src_quality;
    cmd.output_index = buffer_count + channel;
    cmd.flags = voice_info.flags & 3;
    cmd.sample_rate = voice_info.sample_rate;
    cmd.pitch = voice_info.pitch;

    for (u32 i = 0; i < MaxWaveBuffers; i++) {
        voice_info.wavebuffers[i].Copy(cmd.wave_buffers[i]);
    }

    cmd.voice_state = memory_pool_.Translate(CpuAddr(&voice_state), sizeof(VoiceState));
    cmd.data_address = voice_info.data_address.GetReference(true);
    cmd.data_size = voice_info.data_address.GetSize();

    GenerateEnd<AdpcmDataSourceVersion1Command>(cmd);
}

void CommandBuffer::GenerateAdpcmVersion2Command(const s32 node_id, VoiceInfo& voice_info,
                                                 const VoiceState& voice_state,
                                                 const s16 buffer_count, const s8 channel) {
    auto& cmd{
        GenerateStart<AdpcmDataSourceVersion2Command, CommandId::DataSourceAdpcmVersion2>(node_id)};

    cmd.src_quality = voice_info.src_quality;
    cmd.output_index = buffer_count + channel;
    cmd.flags = voice_info.flags & 3;
    cmd.sample_rate = voice_info.sample_rate;
    cmd.pitch = voice_info.pitch;
    cmd.channel_index = channel;
    cmd.channel_count = voice_info.channel_count;

    for (u32 i = 0; i < MaxWaveBuffers; i++) {
        voice_info.wavebuffers[i].Copy(cmd.wave_buffers[i]);
    }

    cmd.voice_state = memory_pool->Translate(CpuAddr(&voice_state), sizeof(VoiceState));
    cmd.data_address = voice_info.data_address.GetReference(true);
    cmd.data_size = voice_info.data_address.GetSize();

    GenerateEnd<AdpcmDataSourceVersion2Command>(cmd);
}

void CommandBuffer::GenerateVolumeCommand(const s32 node_id, const s16 buffer_offset,
                                          const s16 input_index, const f32 volume,
                                          const u8 precision) {
    auto& cmd{GenerateStart<VolumeCommand, CommandId::Volume>(node_id)};

    cmd.precision = precision;
    cmd.input_index = buffer_offset + input_index;
    cmd.output_index = buffer_offset + input_index;
    cmd.volume = volume;

    GenerateEnd<VolumeCommand>(cmd);
}

void CommandBuffer::GenerateVolumeRampCommand(const s32 node_id, VoiceInfo& voice_info,
                                              const s16 buffer_count, const u8 precision) {
    auto& cmd{GenerateStart<VolumeRampCommand, CommandId::VolumeRamp>(node_id)};

    cmd.input_index = buffer_count;
    cmd.output_index = buffer_count;
    cmd.prev_volume = voice_info.prev_volume;
    cmd.volume = voice_info.volume;
    cmd.precision = precision;

    GenerateEnd<VolumeRampCommand>(cmd);
}

void CommandBuffer::GenerateBiquadFilterCommand(const s32 node_id, VoiceInfo& voice_info,
                                                const VoiceState& voice_state,
                                                const s16 buffer_count, const s8 channel,
                                                const u32 biquad_index,
                                                const bool use_float_processing) {
    auto& cmd{GenerateStart<BiquadFilterCommand, CommandId::BiquadFilter>(node_id)};

    cmd.input = buffer_count + channel;
    cmd.output = buffer_count + channel;

    cmd.biquad = voice_info.biquads[biquad_index];

    cmd.state = memory_pool->Translate(CpuAddr(voice_state.biquad_states[biquad_index].data()),
                                       MaxBiquadFilters * sizeof(VoiceState::BiquadFilterState));

    cmd.needs_init = !voice_info.biquad_initialized[biquad_index];
    cmd.use_float_processing = use_float_processing;

    GenerateEnd<BiquadFilterCommand>(cmd);
}

void CommandBuffer::GenerateBiquadFilterCommand(const s32 node_id, EffectInfoBase& effect_info,
                                                const s16 buffer_offset, const s8 channel,
                                                const bool needs_init,
                                                const bool use_float_processing) {
    auto& cmd{GenerateStart<BiquadFilterCommand, CommandId::BiquadFilter>(node_id)};

    const auto& parameter{
        *reinterpret_cast<BiquadFilterInfo::ParameterVersion1*>(effect_info.GetParameter())};
    const auto state{
        reinterpret_cast<VoiceState::BiquadFilterState*>(effect_info.GetStateBuffer())};

    cmd.input = buffer_offset + parameter.inputs[channel];
    cmd.output = buffer_offset + parameter.outputs[channel];

    cmd.biquad.b = parameter.b;
    cmd.biquad.a = parameter.a;

    cmd.state = memory_pool->Translate(CpuAddr(state),
                                       MaxBiquadFilters * sizeof(VoiceState::BiquadFilterState));

    cmd.needs_init = needs_init;
    cmd.use_float_processing = use_float_processing;

    GenerateEnd<BiquadFilterCommand>(cmd);
}

void CommandBuffer::GenerateMixCommand(const s32 node_id, const s16 input_index,
                                       const s16 output_index, const s16 buffer_offset,
                                       const f32 volume, const u8 precision) {
    auto& cmd{GenerateStart<MixCommand, CommandId::Mix>(node_id)};

    cmd.input_index = input_index;
    cmd.output_index = output_index;
    cmd.volume = volume;
    cmd.precision = precision;

    GenerateEnd<MixCommand>(cmd);
}

void CommandBuffer::GenerateMixRampCommand(const s32 node_id,
                                           [[maybe_unused]] const s16 buffer_count,
                                           const s16 input_index, const s16 output_index,
                                           const f32 volume, const f32 prev_volume,
                                           const CpuAddr prev_samples, const u8 precision) {
    if (volume == 0.0f && prev_volume == 0.0f) {
        return;
    }

    auto& cmd{GenerateStart<MixRampCommand, CommandId::MixRamp>(node_id)};

    cmd.input_index = input_index;
    cmd.output_index = output_index;
    cmd.prev_volume = prev_volume;
    cmd.volume = volume;
    cmd.previous_sample = prev_samples;
    cmd.precision = precision;

    GenerateEnd<MixRampCommand>(cmd);
}

void CommandBuffer::GenerateMixRampGroupedCommand(const s32 node_id, const s16 buffer_count,
                                                  const s16 input_index, s16 output_index,
                                                  std::span<const f32> volumes,
                                                  std::span<const f32> prev_volumes,
                                                  const CpuAddr prev_samples, const u8 precision) {
    auto& cmd{GenerateStart<MixRampGroupedCommand, CommandId::MixRampGrouped>(node_id)};

    cmd.buffer_count = buffer_count;

    for (s32 i = 0; i < buffer_count; i++) {
        cmd.inputs[i] = input_index;
        cmd.outputs[i] = output_index++;
        cmd.prev_volumes[i] = prev_volumes[i];
        cmd.volumes[i] = volumes[i];
    }

    cmd.previous_samples = prev_samples;
    cmd.precision = precision;

    GenerateEnd<MixRampGroupedCommand>(cmd);
}

void CommandBuffer::GenerateDepopPrepareCommand(const s32 node_id, const VoiceState& voice_state,
                                                std::span<const s32> buffer, const s16 buffer_count,
                                                s16 buffer_offset, const bool was_playing) {
    auto& cmd{GenerateStart<DepopPrepareCommand, CommandId::DepopPrepare>(node_id)};

    cmd.enabled = was_playing;

    for (u32 i = 0; i < MaxMixBuffers; i++) {
        cmd.inputs[i] = buffer_offset++;
    }

    cmd.previous_samples = memory_pool->Translate(CpuAddr(voice_state.previous_samples.data()),
                                                  MaxMixBuffers * sizeof(s32));
    cmd.buffer_count = buffer_count;
    cmd.depop_buffer = memory_pool->Translate(CpuAddr(buffer.data()), buffer.size_bytes());

    GenerateEnd<DepopPrepareCommand>(cmd);
}

void CommandBuffer::GenerateDepopForMixBuffersCommand(const s32 node_id, const MixInfo& mix_info,
                                                      std::span<const s32> depop_buffer) {
    auto& cmd{GenerateStart<DepopForMixBuffersCommand, CommandId::DepopForMixBuffers>(node_id)};

    cmd.input = mix_info.buffer_offset;
    cmd.count = mix_info.buffer_count;
    cmd.decay = mix_info.sample_rate == TargetSampleRate ? 0.96218872f : 0.94369507f;
    cmd.depop_buffer =
        memory_pool->Translate(CpuAddr(depop_buffer.data()), mix_info.buffer_count * sizeof(s32));

    GenerateEnd<DepopForMixBuffersCommand>(cmd);
}

void CommandBuffer::GenerateDelayCommand(const s32 node_id, EffectInfoBase& effect_info,
                                         const s16 buffer_offset) {
    auto& cmd{GenerateStart<DelayCommand, CommandId::Delay>(node_id)};

    const auto& parameter{
        *reinterpret_cast<DelayInfo::ParameterVersion1*>(effect_info.GetParameter())};
    const auto state{effect_info.GetStateBuffer()};

    if (IsChannelCountValid(parameter.channel_count)) {
        const auto state_buffer{memory_pool->Translate(CpuAddr(state), sizeof(DelayInfo::State))};
        if (state_buffer) {
            for (s16 channel = 0; channel < parameter.channel_count; channel++) {
                cmd.inputs[channel] = buffer_offset + parameter.inputs[channel];
                cmd.outputs[channel] = buffer_offset + parameter.outputs[channel];
            }

            if (!behavior->IsDelayChannelMappingChanged() && parameter.channel_count == 6) {
                UseOldChannelMapping(cmd.inputs, cmd.outputs);
            }

            cmd.parameter = parameter;
            cmd.effect_enabled = effect_info.IsEnabled();
            cmd.state = state_buffer;
            cmd.workbuffer = effect_info.GetWorkbuffer(-1);
        }
    }

    GenerateEnd<DelayCommand>(cmd);
}

void CommandBuffer::GenerateUpsampleCommand(const s32 node_id, const s16 buffer_offset,
                                            UpsamplerInfo& upsampler_info, const u32 input_count,
                                            std::span<const s8> inputs, const s16 buffer_count,
                                            const u32 sample_count_, const u32 sample_rate_) {
    auto& cmd{GenerateStart<UpsampleCommand, CommandId::Upsample>(node_id)};

    cmd.samples_buffer = memory_pool->Translate(upsampler_info.samples_pos,
                                                upsampler_info.sample_count * sizeof(s32));
    cmd.inputs = memory_pool->Translate(CpuAddr(upsampler_info.inputs.data()), MaxChannels);
    cmd.buffer_count = buffer_count;
    cmd.unk_20 = 0;
    cmd.source_sample_count = sample_count_;
    cmd.source_sample_rate = sample_rate_;

    upsampler_info.input_count = input_count;
    for (u32 i = 0; i < input_count; i++) {
        upsampler_info.inputs[i] = buffer_offset + inputs[i];
    }

    cmd.upsampler_info = memory_pool->Translate(CpuAddr(&upsampler_info), sizeof(UpsamplerInfo));

    GenerateEnd<UpsampleCommand>(cmd);
}

void CommandBuffer::GenerateDownMix6chTo2chCommand(const s32 node_id, std::span<const s8> inputs,
                                                   const s16 buffer_offset,
                                                   std::span<const f32> downmix_coeff) {
    auto& cmd{GenerateStart<DownMix6chTo2chCommand, CommandId::DownMix6chTo2ch>(node_id)};

    for (u32 i = 0; i < MaxChannels; i++) {
        cmd.inputs[i] = buffer_offset + inputs[i];
        cmd.outputs[i] = buffer_offset + inputs[i];
    }

    for (u32 i = 0; i < 4; i++) {
        cmd.down_mix_coeff[i] = downmix_coeff[i];
    }

    GenerateEnd<DownMix6chTo2chCommand>(cmd);
}

void CommandBuffer::GenerateAuxCommand(const s32 node_id, EffectInfoBase& effect_info,
                                       const s16 input_index, const s16 output_index,
                                       const s16 buffer_offset, const u32 update_count,
                                       const u32 count_max, const u32 write_offset) {
    auto& cmd{GenerateStart<AuxCommand, CommandId::Aux>(node_id)};

    if (effect_info.GetSendBuffer() != 0 && effect_info.GetReturnBuffer() != 0) {
        cmd.input = buffer_offset + input_index;
        cmd.output = buffer_offset + output_index;
        cmd.send_buffer_info = effect_info.GetSendBufferInfo();
        cmd.send_buffer = effect_info.GetSendBuffer();
        cmd.return_buffer_info = effect_info.GetReturnBufferInfo();
        cmd.return_buffer = effect_info.GetReturnBuffer();
        cmd.count_max = count_max;
        cmd.write_offset = write_offset;
        cmd.update_count = update_count;
        cmd.effect_enabled = effect_info.IsEnabled();
    }

    GenerateEnd<AuxCommand>(cmd);
}

void CommandBuffer::GenerateDeviceSinkCommand(const s32 node_id, const s16 buffer_offset,
                                              SinkInfoBase& sink_info, const u32 session_id,
                                              std::span<s32> samples_buffer) {
    auto& cmd{GenerateStart<DeviceSinkCommand, CommandId::DeviceSink>(node_id)};
    const auto& parameter{
        *reinterpret_cast<DeviceSinkInfo::DeviceInParameter*>(sink_info.GetParameter())};
    auto state{*reinterpret_cast<DeviceSinkInfo::DeviceState*>(sink_info.GetState())};

    cmd.session_id = session_id;

    cmd.input_count = parameter.input_count;
    s16 max_input{0};
    for (u32 i = 0; i < parameter.input_count; i++) {
        cmd.inputs[i] = buffer_offset + parameter.inputs[i];
        max_input = std::max(max_input, cmd.inputs[i]);
    }

    if (state.upsampler_info != nullptr) {
        const auto size_{state.upsampler_info->sample_count * parameter.input_count};
        const auto size_bytes{size_ * sizeof(s32)};
        const auto addr{memory_pool->Translate(state.upsampler_info->samples_pos, size_bytes)};
        cmd.sample_buffer = {reinterpret_cast<s32*>(addr),
                             (max_input + 1) * state.upsampler_info->sample_count};
    } else {
        cmd.sample_buffer = samples_buffer;
    }

    GenerateEnd<DeviceSinkCommand>(cmd);
}

void CommandBuffer::GenerateCircularBufferSinkCommand(const s32 node_id, SinkInfoBase& sink_info,
                                                      const s16 buffer_offset) {
    auto& cmd{GenerateStart<CircularBufferSinkCommand, CommandId::CircularBufferSink>(node_id)};
    const auto& parameter{*reinterpret_cast<CircularBufferSinkInfo::CircularBufferInParameter*>(
        sink_info.GetParameter())};
    auto state{
        *reinterpret_cast<CircularBufferSinkInfo::CircularBufferState*>(sink_info.GetState())};

    cmd.input_count = parameter.input_count;
    for (u32 i = 0; i < parameter.input_count; i++) {
        cmd.inputs[i] = buffer_offset + parameter.inputs[i];
    }

    cmd.address = state.address_info.GetReference(true);
    cmd.size = parameter.size;
    cmd.pos = state.current_pos;

    GenerateEnd<CircularBufferSinkCommand>(cmd);
}

void CommandBuffer::GenerateReverbCommand(const s32 node_id, EffectInfoBase& effect_info,
                                          const s16 buffer_offset,
                                          const bool long_size_pre_delay_supported) {
    auto& cmd{GenerateStart<ReverbCommand, CommandId::Reverb>(node_id)};

    const auto& parameter{
        *reinterpret_cast<ReverbInfo::ParameterVersion2*>(effect_info.GetParameter())};
    const auto state{effect_info.GetStateBuffer()};

    if (IsChannelCountValid(parameter.channel_count)) {
        const auto state_buffer{memory_pool->Translate(CpuAddr(state), sizeof(ReverbInfo::State))};
        if (state_buffer) {
            for (s16 channel = 0; channel < parameter.channel_count; channel++) {
                cmd.inputs[channel] = buffer_offset + parameter.inputs[channel];
                cmd.outputs[channel] = buffer_offset + parameter.outputs[channel];
            }

            if (!behavior->IsReverbChannelMappingChanged() && parameter.channel_count == 6) {
                UseOldChannelMapping(cmd.inputs, cmd.outputs);
            }

            cmd.parameter = parameter;
            cmd.effect_enabled = effect_info.IsEnabled();
            cmd.state = state_buffer;
            cmd.workbuffer = effect_info.GetWorkbuffer(-1);
            cmd.long_size_pre_delay_supported = long_size_pre_delay_supported;
        }
    }

    GenerateEnd<ReverbCommand>(cmd);
}

void CommandBuffer::GenerateI3dl2ReverbCommand(const s32 node_id, EffectInfoBase& effect_info,
                                               const s16 buffer_offset) {
    auto& cmd{GenerateStart<I3dl2ReverbCommand, CommandId::I3dl2Reverb>(node_id)};

    const auto& parameter{
        *reinterpret_cast<I3dl2ReverbInfo::ParameterVersion1*>(effect_info.GetParameter())};
    const auto state{effect_info.GetStateBuffer()};

    if (IsChannelCountValid(parameter.channel_count)) {
        const auto state_buffer{
            memory_pool->Translate(CpuAddr(state), sizeof(I3dl2ReverbInfo::State))};
        if (state_buffer) {
            for (s16 channel = 0; channel < parameter.channel_count; channel++) {
                cmd.inputs[channel] = buffer_offset + parameter.inputs[channel];
                cmd.outputs[channel] = buffer_offset + parameter.outputs[channel];
            }

            if (!behavior->IsI3dl2ReverbChannelMappingChanged() && parameter.channel_count == 6) {
                UseOldChannelMapping(cmd.inputs, cmd.outputs);
            }

            cmd.parameter = parameter;
            cmd.effect_enabled = effect_info.IsEnabled();
            cmd.state = state_buffer;
            cmd.workbuffer = effect_info.GetWorkbuffer(-1);
        }
    }

    GenerateEnd<I3dl2ReverbCommand>(cmd);
}

void CommandBuffer::GeneratePerformanceCommand(const s32 node_id, const PerformanceState state,
                                               const PerformanceEntryAddresses& entry_addresses) {
    auto& cmd{GenerateStart<PerformanceCommand, CommandId::Performance>(node_id)};

    cmd.state = state;
    cmd.entry_address = entry_addresses;

    GenerateEnd<PerformanceCommand>(cmd);
}

void CommandBuffer::GenerateClearMixCommand(const s32 node_id) {
    auto& cmd{GenerateStart<ClearMixBufferCommand, CommandId::ClearMixBuffer>(node_id)};
    GenerateEnd<ClearMixBufferCommand>(cmd);
}

void CommandBuffer::GenerateCopyMixBufferCommand(const s32 node_id, EffectInfoBase& effect_info,
                                                 const s16 buffer_offset, const s8 channel) {
    auto& cmd{GenerateStart<CopyMixBufferCommand, CommandId::CopyMixBuffer>(node_id)};

    const auto& parameter{
        *reinterpret_cast<BiquadFilterInfo::ParameterVersion1*>(effect_info.GetParameter())};
    cmd.input_index = buffer_offset + parameter.inputs[channel];
    cmd.output_index = buffer_offset + parameter.outputs[channel];

    GenerateEnd<CopyMixBufferCommand>(cmd);
}

void CommandBuffer::GenerateLightLimiterCommand(
    const s32 node_id, const s16 buffer_offset,
    const LightLimiterInfo::ParameterVersion1& parameter, const LightLimiterInfo::State& state,
    const bool enabled, const CpuAddr workbuffer) {
    auto& cmd{GenerateStart<LightLimiterVersion1Command, CommandId::LightLimiterVersion1>(node_id)};

    if (IsChannelCountValid(parameter.channel_count)) {
        const auto state_buffer{
            memory_pool->Translate(CpuAddr(&state), sizeof(LightLimiterInfo::State))};
        if (state_buffer) {
            for (s8 channel = 0; channel < parameter.channel_count; channel++) {
                cmd.inputs[channel] = buffer_offset + parameter.inputs[channel];
                cmd.outputs[channel] = buffer_offset + parameter.outputs[channel];
            }

            std::memcpy(&cmd.parameter, &parameter, sizeof(LightLimiterInfo::ParameterVersion1));
            cmd.effect_enabled = enabled;
            cmd.state = state_buffer;
            cmd.workbuffer = workbuffer;
        }
    }

    GenerateEnd<LightLimiterVersion1Command>(cmd);
}

void CommandBuffer::GenerateLightLimiterCommand(
    const s32 node_id, const s16 buffer_offset,
    const LightLimiterInfo::ParameterVersion2& parameter,
    const LightLimiterInfo::StatisticsInternal& statistics, const LightLimiterInfo::State& state,
    const bool enabled, const CpuAddr workbuffer) {
    auto& cmd{GenerateStart<LightLimiterVersion2Command, CommandId::LightLimiterVersion2>(node_id)};
    if (IsChannelCountValid(parameter.channel_count)) {
        const auto state_buffer{
            memory_pool->Translate(CpuAddr(&state), sizeof(LightLimiterInfo::State))};
        if (state_buffer) {
            for (s8 channel = 0; channel < parameter.channel_count; channel++) {
                cmd.inputs[channel] = buffer_offset + parameter.inputs[channel];
                cmd.outputs[channel] = buffer_offset + parameter.outputs[channel];
            }

            cmd.parameter = parameter;
            cmd.effect_enabled = enabled;
            cmd.state = state_buffer;
            if (cmd.parameter.statistics_enabled) {
                cmd.result_state = memory_pool->Translate(
                    CpuAddr(&statistics), sizeof(LightLimiterInfo::StatisticsInternal));
            } else {
                cmd.result_state = 0;
            }
            cmd.workbuffer = workbuffer;
        }
    }

    GenerateEnd<LightLimiterVersion2Command>(cmd);
}

void CommandBuffer::GenerateMultitapBiquadFilterCommand(const s32 node_id, VoiceInfo& voice_info,
                                                        const VoiceState& voice_state,
                                                        const s16 buffer_count, const s8 channel) {
    auto& cmd{GenerateStart<MultiTapBiquadFilterCommand, CommandId::MultiTapBiquadFilter>(node_id)};

    cmd.input = buffer_count + channel;
    cmd.output = buffer_count + channel;
    cmd.biquads = voice_info.biquads;

    cmd.states[0] =
        memory_pool->Translate(CpuAddr(voice_state.biquad_states[0].data()),
                               MaxBiquadFilters * sizeof(VoiceState::BiquadFilterState));
    cmd.states[1] =
        memory_pool->Translate(CpuAddr(voice_state.biquad_states[1].data()),
                               MaxBiquadFilters * sizeof(VoiceState::BiquadFilterState));

    cmd.needs_init[0] = !voice_info.biquad_initialized[0];
    cmd.needs_init[1] = !voice_info.biquad_initialized[1];
    cmd.filter_tap_count = MaxBiquadFilters;

    GenerateEnd<MultiTapBiquadFilterCommand>(cmd);
}

void CommandBuffer::GenerateCaptureCommand(const s32 node_id, EffectInfoBase& effect_info,
                                           const s16 input_index, const s16 output_index,
                                           const s16 buffer_offset, const u32 update_count,
                                           const u32 count_max, const u32 write_offset) {
    auto& cmd{GenerateStart<CaptureCommand, CommandId::Capture>(node_id)};

    if (effect_info.GetSendBuffer()) {
        cmd.input = buffer_offset + input_index;
        cmd.output = buffer_offset + output_index;
        cmd.send_buffer_info = effect_info.GetSendBufferInfo();
        cmd.send_buffer = effect_info.GetSendBuffer();
        cmd.count_max = count_max;
        cmd.write_offset = write_offset;
        cmd.update_count = update_count;
        cmd.effect_enabled = effect_info.IsEnabled();
    }

    GenerateEnd<CaptureCommand>(cmd);
}

void CommandBuffer::GenerateCompressorCommand(s16 buffer_offset, EffectInfoBase& effect_info,
                                              s32 node_id) {
    auto& cmd{GenerateStart<CompressorCommand, CommandId::Compressor>(node_id)};

    auto& parameter{
        *reinterpret_cast<CompressorInfo::ParameterVersion2*>(effect_info.GetParameter())};
    auto state{reinterpret_cast<CompressorInfo::State*>(effect_info.GetStateBuffer())};

    if (IsChannelCountValid(parameter.channel_count)) {
        auto state_buffer{memory_pool->Translate(CpuAddr(state), sizeof(CompressorInfo::State))};
        if (state_buffer) {
            for (u16 channel = 0; channel < parameter.channel_count; channel++) {
                cmd.inputs[channel] = buffer_offset + parameter.inputs[channel];
                cmd.outputs[channel] = buffer_offset + parameter.outputs[channel];
            }
            cmd.parameter = parameter;
            cmd.workbuffer = state_buffer;
            cmd.enabled = effect_info.IsEnabled();
        }
    }

    GenerateEnd<CompressorCommand>(cmd);
}

} // namespace AudioCore::AudioRenderer
