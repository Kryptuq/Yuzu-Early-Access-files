// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/common/audio_renderer_parameter.h"
#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/command/command_buffer.h"
#include "audio_core/renderer/command/command_generator.h"
#include "audio_core/renderer/command/command_list_header.h"
#include "audio_core/renderer/effect/aux_.h"
#include "audio_core/renderer/effect/biquad_filter.h"
#include "audio_core/renderer/effect/buffer_mixer.h"
#include "audio_core/renderer/effect/capture.h"
#include "audio_core/renderer/effect/effect_context.h"
#include "audio_core/renderer/effect/light_limiter.h"
#include "audio_core/renderer/mix/mix_context.h"
#include "audio_core/renderer/performance/detail_aspect.h"
#include "audio_core/renderer/performance/entry_aspect.h"
#include "audio_core/renderer/sink/device_sink_info.h"
#include "audio_core/renderer/sink/sink_context.h"
#include "audio_core/renderer/splitter/splitter_context.h"
#include "audio_core/renderer/voice/voice_context.h"
#include "common/alignment.h"

namespace AudioCore::AudioRenderer {

CommandGenerator::CommandGenerator(CommandBuffer& command_buffer_,
                                   const CommandListHeader& command_list_header_,
                                   const AudioRendererSystemContext& render_context_,
                                   VoiceContext& voice_context_, MixContext& mix_context_,
                                   EffectContext& effect_context_, SinkContext& sink_context_,
                                   SplitterContext& splitter_context_,
                                   PerformanceManager* performance_manager_)
    : command_buffer{command_buffer_}, command_header{command_list_header_},
      render_context{render_context_}, voice_context{voice_context_}, mix_context{mix_context_},
      effect_context{effect_context_}, sink_context{sink_context_},
      splitter_context{splitter_context_}, performance_manager{performance_manager_} {
    command_buffer.GenerateClearMixCommand(InvalidNodeId);
}

void CommandGenerator::GenerateDataSourceCommand(VoiceInfo& voice_info,
                                                 const VoiceState& voice_state, const s8 channel) {
    if (voice_info.mix_id == UnusedMixId) {
        if (voice_info.splitter_id != UnusedSplitterId) {
            auto destination{splitter_context.GetDesintationData(voice_info.splitter_id, 0)};
            u32 dest_id{0};
            while (destination != nullptr) {
                if (destination->IsConfigured()) {
                    auto mix_id{destination->GetMixId()};
                    if (mix_id < mix_context.GetCount() && mix_id != -1) {
                        auto mix_info{mix_context.GetInfo(mix_id)};
                        command_buffer.GenerateDepopPrepareCommand(
                            voice_info.node_id, voice_state, render_context.depop_buffer,
                            mix_info->buffer_count, mix_info->buffer_offset,
                            voice_info.was_playing);
                    }
                }
                dest_id++;
                destination = splitter_context.GetDesintationData(voice_info.splitter_id, dest_id);
            }
        }
    } else {
        auto mix_info{mix_context.GetInfo(voice_info.mix_id)};
        command_buffer.GenerateDepopPrepareCommand(
            voice_info.node_id, voice_state, render_context.depop_buffer, mix_info->buffer_count,
            mix_info->buffer_offset, voice_info.was_playing);
    }

    if (voice_info.was_playing) {
        return;
    }

    if (render_context.behavior->IsWaveBufferVer2Supported()) {
        switch (voice_info.sample_format) {
        case SampleFormat::PcmInt16:
            command_buffer.GeneratePcmInt16Version2Command(
                voice_info.node_id, voice_info, voice_state, render_context.mix_buffer_count,
                channel);
            break;
        case SampleFormat::PcmFloat:
            command_buffer.GeneratePcmFloatVersion2Command(
                voice_info.node_id, voice_info, voice_state, render_context.mix_buffer_count,
                channel);
            break;
        case SampleFormat::Adpcm:
            command_buffer.GenerateAdpcmVersion2Command(voice_info.node_id, voice_info, voice_state,
                                                        render_context.mix_buffer_count, channel);
            break;
        default:
            LOG_ERROR(Service_Audio, "Invalid SampleFormat {}",
                      static_cast<u32>(voice_info.sample_format));
            break;
        }
    } else {
        switch (voice_info.sample_format) {
        case SampleFormat::PcmInt16:
            command_buffer.GeneratePcmInt16Version1Command(
                voice_info.node_id, *command_buffer.memory_pool, voice_info, voice_state,
                render_context.mix_buffer_count, channel);
            break;
        case SampleFormat::PcmFloat:
            command_buffer.GeneratePcmFloatVersion1Command(
                voice_info.node_id, *command_buffer.memory_pool, voice_info, voice_state,
                render_context.mix_buffer_count, channel);
            break;
        case SampleFormat::Adpcm:
            command_buffer.GenerateAdpcmVersion1Command(
                voice_info.node_id, *command_buffer.memory_pool, voice_info, voice_state,
                render_context.mix_buffer_count, channel);
            break;
        default:
            LOG_ERROR(Service_Audio, "Invalid SampleFormat {}",
                      static_cast<u32>(voice_info.sample_format));
            break;
        }
    }
}

void CommandGenerator::GenerateVoiceMixCommand(std::span<const f32> mix_volumes,
                                               std::span<const f32> prev_mix_volumes,
                                               const VoiceState& voice_state, s16 output_index,
                                               const s16 buffer_count, const s16 input_index,
                                               const s32 node_id) {
    u8 precision{15};
    if (render_context.behavior->IsVolumeMixParameterPrecisionQ23Supported()) {
        precision = 23;
    }

    if (buffer_count > 8) {
        const auto prev_samples{render_context.memory_pool_info->Translate(
            CpuAddr(voice_state.previous_samples.data()), buffer_count * sizeof(s32))};
        command_buffer.GenerateMixRampGroupedCommand(node_id, buffer_count, input_index,
                                                     output_index, mix_volumes, prev_mix_volumes,
                                                     prev_samples, precision);
    } else {
        for (s16 i = 0; i < buffer_count; i++, output_index++) {
            const auto prev_samples{render_context.memory_pool_info->Translate(
                CpuAddr(&voice_state.previous_samples[i]), sizeof(s32))};

            command_buffer.GenerateMixRampCommand(node_id, buffer_count, input_index, output_index,
                                                  mix_volumes[i], prev_mix_volumes[i], prev_samples,
                                                  precision);
        }
    }
}

void CommandGenerator::GenerateBiquadFilterCommandForVoice(VoiceInfo& voice_info,
                                                           const VoiceState& voice_state,
                                                           const s16 buffer_count, const s8 channel,
                                                           const s32 node_id) {
    const bool both_biquads_enabled{voice_info.biquads[0].enabled && voice_info.biquads[1].enabled};
    const auto use_float_processing{render_context.behavior->UseBiquadFilterFloatProcessing()};

    if (both_biquads_enabled && render_context.behavior->UseMultiTapBiquadFilterProcessing() &&
        use_float_processing) {
        command_buffer.GenerateMultitapBiquadFilterCommand(node_id, voice_info, voice_state,
                                                           buffer_count, channel);
    } else {
        for (u32 i = 0; i < MaxBiquadFilters; i++) {
            if (voice_info.biquads[i].enabled) {
                command_buffer.GenerateBiquadFilterCommand(node_id, voice_info, voice_state,
                                                           buffer_count, channel, i,
                                                           use_float_processing);
            }
        }
    }
}

void CommandGenerator::GenerateVoiceCommand(VoiceInfo& voice_info) {
    u8 precision{15};
    if (render_context.behavior->IsVolumeMixParameterPrecisionQ23Supported()) {
        precision = 23;
    }

    for (s8 channel = 0; channel < voice_info.channel_count; channel++) {
        const auto resource_id{voice_info.channel_resource_ids[channel]};
        auto& voice_state{voice_context.GetDspSharedState(resource_id)};
        auto& channel_resource{voice_context.GetChannelResource(resource_id)};

        PerformanceDetailType detail_type{PerformanceDetailType::Invalid};
        switch (voice_info.sample_format) {
        case SampleFormat::PcmInt16:
            detail_type = PerformanceDetailType::Unk1;
            break;
        case SampleFormat::PcmFloat:
            detail_type = PerformanceDetailType::Unk10;
            break;
        default:
            detail_type = PerformanceDetailType::Unk2;
            break;
        }

        DetailAspect data_source_detail(*this, PerformanceEntryType::Voice, voice_info.node_id,
                                        detail_type);
        GenerateDataSourceCommand(voice_info, voice_state, channel);

        if (data_source_detail.initialized) {
            command_buffer.GeneratePerformanceCommand(data_source_detail.node_id,
                                                      PerformanceState::Stop,
                                                      data_source_detail.performance_entry_address);
        }

        if (voice_info.was_playing) {
            voice_info.prev_volume = 0.0f;
            continue;
        }

        if (!voice_info.HasAnyConnection()) {
            continue;
        }

        DetailAspect biquad_detail_aspect(*this, PerformanceEntryType::Voice, voice_info.node_id,
                                          PerformanceDetailType::Unk4);
        GenerateBiquadFilterCommandForVoice(
            voice_info, voice_state, render_context.mix_buffer_count, channel, voice_info.node_id);

        if (biquad_detail_aspect.initialized) {
            command_buffer.GeneratePerformanceCommand(
                biquad_detail_aspect.node_id, PerformanceState::Stop,
                biquad_detail_aspect.performance_entry_address);
        }

        DetailAspect volume_ramp_detail_aspect(*this, PerformanceEntryType::Voice,
                                               voice_info.node_id, PerformanceDetailType::Unk3);
        command_buffer.GenerateVolumeRampCommand(
            voice_info.node_id, voice_info, render_context.mix_buffer_count + channel, precision);
        if (volume_ramp_detail_aspect.initialized) {
            command_buffer.GeneratePerformanceCommand(
                volume_ramp_detail_aspect.node_id, PerformanceState::Stop,
                volume_ramp_detail_aspect.performance_entry_address);
        }

        voice_info.prev_volume = voice_info.volume;

        if (voice_info.mix_id == UnusedMixId) {
            if (voice_info.splitter_id != UnusedSplitterId) {
                auto i{channel};
                auto destination{splitter_context.GetDesintationData(voice_info.splitter_id, i)};
                while (destination != nullptr) {
                    if (destination->IsConfigured()) {
                        const auto mix_id{destination->GetMixId()};
                        if (mix_id < mix_context.GetCount() &&
                            static_cast<s32>(mix_id) != UnusedSplitterId) {
                            auto mix_info{mix_context.GetInfo(mix_id)};
                            GenerateVoiceMixCommand(
                                destination->GetMixVolume(), destination->GetMixVolumePrev(),
                                voice_state, mix_info->buffer_offset, mix_info->buffer_count,
                                render_context.mix_buffer_count + channel, voice_info.node_id);
                            destination->MarkAsNeedToUpdateInternalState();
                        }
                    }
                    i += voice_info.channel_count;
                    destination = splitter_context.GetDesintationData(voice_info.splitter_id, i);
                }
            }
        } else {
            DetailAspect volume_mix_detail_aspect(*this, PerformanceEntryType::Voice,
                                                  voice_info.node_id, PerformanceDetailType::Unk3);
            auto mix_info{mix_context.GetInfo(voice_info.mix_id)};
            GenerateVoiceMixCommand(channel_resource.mix_volumes, channel_resource.prev_mix_volumes,
                                    voice_state, mix_info->buffer_offset, mix_info->buffer_count,
                                    render_context.mix_buffer_count + channel, voice_info.node_id);
            if (volume_mix_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    volume_mix_detail_aspect.node_id, PerformanceState::Stop,
                    volume_mix_detail_aspect.performance_entry_address);
            }

            channel_resource.prev_mix_volumes = channel_resource.mix_volumes;
        }
        voice_info.biquad_initialized[0] = voice_info.biquads[0].enabled;
        voice_info.biquad_initialized[1] = voice_info.biquads[1].enabled;
    }
}

void CommandGenerator::GenerateVoiceCommands() {
    const auto voice_count{voice_context.GetCount()};

    for (u32 i = 0; i < voice_count; i++) {
        auto sorted_info{voice_context.GetSortedInfo(i)};

        if (sorted_info->ShouldSkip() || !sorted_info->UpdateForCommandGeneration(voice_context)) {
            continue;
        }

        EntryAspect voice_entry_aspect(*this, PerformanceEntryType::Voice, sorted_info->node_id);

        GenerateVoiceCommand(*sorted_info);

        if (voice_entry_aspect.initialized) {
            command_buffer.GeneratePerformanceCommand(voice_entry_aspect.node_id,
                                                      PerformanceState::Stop,
                                                      voice_entry_aspect.performance_entry_address);
        }
    }

    splitter_context.UpdateInternalState();
}

void CommandGenerator::GenerateBufferMixerCommand(const s16 buffer_offset,
                                                  EffectInfoBase& effect_info, const s32 node_id) {
    u8 precision{15};
    if (render_context.behavior->IsVolumeMixParameterPrecisionQ23Supported()) {
        precision = 23;
    }

    if (effect_info.IsEnabled()) {
        const auto& parameter{
            *reinterpret_cast<BufferMixerInfo::ParameterVersion1*>(effect_info.GetParameter())};
        for (u32 i = 0; i < parameter.mix_count; i++) {
            if (parameter.volumes[i] != 0.0f) {
                command_buffer.GenerateMixCommand(node_id, buffer_offset + parameter.inputs[i],
                                                  buffer_offset + parameter.outputs[i],
                                                  buffer_offset, parameter.volumes[i], precision);
            }
        }
    }
}

void CommandGenerator::GenerateDelayCommand(const s16 buffer_offset, EffectInfoBase& effect_info,
                                            const s32 node_id) {
    command_buffer.GenerateDelayCommand(node_id, effect_info, buffer_offset);
}

void CommandGenerator::GenerateReverbCommand(const s16 buffer_offset, EffectInfoBase& effect_info,
                                             const s32 node_id,
                                             const bool long_size_pre_delay_supported) {
    command_buffer.GenerateReverbCommand(node_id, effect_info, buffer_offset,
                                         long_size_pre_delay_supported);
}

void CommandGenerator::GenerateI3dl2ReverbEffectCommand(const s16 buffer_offset,
                                                        EffectInfoBase& effect_info,
                                                        const s32 node_id) {
    command_buffer.GenerateI3dl2ReverbCommand(node_id, effect_info, buffer_offset);
}

void CommandGenerator::GenerateAuxCommand(const s16 buffer_offset, EffectInfoBase& effect_info,
                                          const s32 node_id) {

    if (effect_info.IsEnabled()) {
        effect_info.GetWorkbuffer(0);
        effect_info.GetWorkbuffer(1);
    }

    if (effect_info.GetSendBuffer() != 0 && effect_info.GetReturnBuffer() != 0) {
        const auto& parameter{
            *reinterpret_cast<AuxInfo::ParameterVersion1*>(effect_info.GetParameter())};
        auto channel_index{parameter.mix_buffer_count - 1};
        u32 write_offset{0};
        for (u32 i = 0; i < parameter.mix_buffer_count; i++, channel_index--) {
            auto new_update_count{command_header.sample_count + write_offset};
            const auto update_count{channel_index > 0 ? 0 : new_update_count};
            command_buffer.GenerateAuxCommand(node_id, effect_info, parameter.inputs[i],
                                              parameter.outputs[i], buffer_offset, update_count,
                                              parameter.count_max, write_offset);
            write_offset = new_update_count;
        }
    }
}

void CommandGenerator::GenerateBiquadFilterEffectCommand(const s16 buffer_offset,
                                                         EffectInfoBase& effect_info,
                                                         const s32 node_id) {
    const auto& parameter{
        *reinterpret_cast<BiquadFilterInfo::ParameterVersion1*>(effect_info.GetParameter())};
    if (effect_info.IsEnabled()) {
        bool needs_init{false};

        switch (parameter.state) {
        case EffectInfoBase::ParameterState::Initialized:
            needs_init = true;
            break;
        case EffectInfoBase::ParameterState::Updating:
        case EffectInfoBase::ParameterState::Updated:
            if (render_context.behavior->IsBiquadFilterEffectStateClearBugFixed()) {
                needs_init = false;
            } else {
                needs_init = parameter.state == EffectInfoBase::ParameterState::Updating;
            }
            break;
        default:
            LOG_ERROR(Service_Audio, "Invalid biquad parameter state {}",
                      static_cast<u32>(parameter.state));
            break;
        }

        for (s8 channel = 0; channel < parameter.channel_count; channel++) {
            command_buffer.GenerateBiquadFilterCommand(
                node_id, effect_info, buffer_offset, channel, needs_init,
                render_context.behavior->UseBiquadFilterFloatProcessing());
        }
    } else {
        for (s8 channel = 0; channel < parameter.channel_count; channel++) {
            command_buffer.GenerateCopyMixBufferCommand(node_id, effect_info, buffer_offset,
                                                        channel);
        }
    }
}

void CommandGenerator::GenerateLightLimiterEffectCommand(const s16 buffer_offset,
                                                         EffectInfoBase& effect_info,
                                                         const s32 node_id,
                                                         const u32 effect_index) {

    const auto& state{*reinterpret_cast<LightLimiterInfo::State*>(effect_info.GetStateBuffer())};

    if (render_context.behavior->IsEffectInfoVersion2Supported()) {
        const auto& parameter{
            *reinterpret_cast<LightLimiterInfo::ParameterVersion2*>(effect_info.GetParameter())};
        const auto& result_state{*reinterpret_cast<LightLimiterInfo::StatisticsInternal*>(
            &effect_context.GetDspSharedResultState(effect_index))};
        command_buffer.GenerateLightLimiterCommand(node_id, buffer_offset, parameter, result_state,
                                                   state, effect_info.IsEnabled(),
                                                   effect_info.GetWorkbuffer(-1));
    } else {
        const auto& parameter{
            *reinterpret_cast<LightLimiterInfo::ParameterVersion1*>(effect_info.GetParameter())};
        command_buffer.GenerateLightLimiterCommand(node_id, buffer_offset, parameter, state,
                                                   effect_info.IsEnabled(),
                                                   effect_info.GetWorkbuffer(-1));
    }
}

void CommandGenerator::GenerateCaptureCommand(const s16 buffer_offset, EffectInfoBase& effect_info,
                                              const s32 node_id) {
    if (effect_info.IsEnabled()) {
        effect_info.GetWorkbuffer(0);
    }

    if (effect_info.GetSendBuffer()) {
        const auto& parameter{
            *reinterpret_cast<AuxInfo::ParameterVersion1*>(effect_info.GetParameter())};
        auto channel_index{parameter.mix_buffer_count - 1};
        u32 write_offset{0};
        for (u32 i = 0; i < parameter.mix_buffer_count; i++, channel_index--) {
            auto new_update_count{command_header.sample_count + write_offset};
            const auto update_count{channel_index > 0 ? 0 : new_update_count};
            command_buffer.GenerateCaptureCommand(node_id, effect_info, parameter.inputs[i],
                                                  parameter.outputs[i], buffer_offset, update_count,
                                                  parameter.count_max, write_offset);
            write_offset = new_update_count;
        }
    }
}

void CommandGenerator::GenerateCompressorCommand(const s16 buffer_offset,
                                                 EffectInfoBase& effect_info, const s32 node_id) {
    command_buffer.GenerateCompressorCommand(buffer_offset, effect_info, node_id);
}

void CommandGenerator::GenerateEffectCommand(MixInfo& mix_info) {
    const auto effect_count{effect_context.GetCount()};
    for (u32 i = 0; i < effect_count; i++) {
        const auto effect_index{mix_info.effect_order_buffer[i]};
        if (effect_index == -1) {
            break;
        }

        auto& effect_info = effect_context.GetInfo(effect_index);
        if (effect_info.ShouldSkip()) {
            continue;
        }

        const auto entry_type{mix_info.mix_id == FinalMixId ? PerformanceEntryType::FinalMix
                                                            : PerformanceEntryType::SubMix};

        switch (effect_info.GetType()) {
        case EffectInfoBase::Type::Mix: {
            DetailAspect mix_detail_aspect(*this, entry_type, mix_info.node_id,
                                           PerformanceDetailType::Unk5);
            GenerateBufferMixerCommand(mix_info.buffer_offset, effect_info, mix_info.node_id);
            if (mix_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    mix_detail_aspect.node_id, PerformanceState::Stop,
                    mix_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::Aux: {
            DetailAspect aux_detail_aspect(*this, entry_type, mix_info.node_id,
                                           PerformanceDetailType::Unk7);
            GenerateAuxCommand(mix_info.buffer_offset, effect_info, mix_info.node_id);
            if (aux_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    aux_detail_aspect.node_id, PerformanceState::Stop,
                    aux_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::Delay: {
            DetailAspect delay_detail_aspect(*this, entry_type, mix_info.node_id,
                                             PerformanceDetailType::Unk6);
            GenerateDelayCommand(mix_info.buffer_offset, effect_info, mix_info.node_id);
            if (delay_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    delay_detail_aspect.node_id, PerformanceState::Stop,
                    delay_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::Reverb: {
            DetailAspect reverb_detail_aspect(*this, entry_type, mix_info.node_id,
                                              PerformanceDetailType::Unk8);
            GenerateReverbCommand(mix_info.buffer_offset, effect_info, mix_info.node_id,
                                  render_context.behavior->IsLongSizePreDelaySupported());
            if (reverb_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    reverb_detail_aspect.node_id, PerformanceState::Stop,
                    reverb_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::I3dl2Reverb: {
            DetailAspect i3dl2_detail_aspect(*this, entry_type, mix_info.node_id,
                                             PerformanceDetailType::Unk9);
            GenerateI3dl2ReverbEffectCommand(mix_info.buffer_offset, effect_info, mix_info.node_id);
            if (i3dl2_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    i3dl2_detail_aspect.node_id, PerformanceState::Stop,
                    i3dl2_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::BiquadFilter: {
            DetailAspect biquad_detail_aspect(*this, entry_type, mix_info.node_id,
                                              PerformanceDetailType::Unk4);
            GenerateBiquadFilterEffectCommand(mix_info.buffer_offset, effect_info,
                                              mix_info.node_id);
            if (biquad_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    biquad_detail_aspect.node_id, PerformanceState::Stop,
                    biquad_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::LightLimiter: {
            DetailAspect light_limiter_detail_aspect(*this, entry_type, mix_info.node_id,
                                                     PerformanceDetailType::Unk11);
            GenerateLightLimiterEffectCommand(mix_info.buffer_offset, effect_info, mix_info.node_id,
                                              effect_index);
            if (light_limiter_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    light_limiter_detail_aspect.node_id, PerformanceState::Stop,
                    light_limiter_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::Capture: {
            DetailAspect capture_detail_aspect(*this, entry_type, mix_info.node_id,
                                               PerformanceDetailType::Unk12);
            GenerateCaptureCommand(mix_info.buffer_offset, effect_info, mix_info.node_id);
            if (capture_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    capture_detail_aspect.node_id, PerformanceState::Stop,
                    capture_detail_aspect.performance_entry_address);
            }
        } break;

        case EffectInfoBase::Type::Compressor: {
            DetailAspect capture_detail_aspect(*this, entry_type, mix_info.node_id,
                                               PerformanceDetailType::Unk13);
            GenerateCompressorCommand(mix_info.buffer_offset, effect_info, mix_info.node_id);
            if (capture_detail_aspect.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    capture_detail_aspect.node_id, PerformanceState::Stop,
                    capture_detail_aspect.performance_entry_address);
            }
        } break;

        default:
            LOG_ERROR(Service_Audio, "Invalid effect type {}",
                      static_cast<u32>(effect_info.GetType()));
            break;
        }

        effect_info.UpdateForCommandGeneration();
    }
}

void CommandGenerator::GenerateMixCommands(MixInfo& mix_info) {
    u8 precision{15};
    if (render_context.behavior->IsVolumeMixParameterPrecisionQ23Supported()) {
        precision = 23;
    }

    if (!mix_info.HasAnyConnection()) {
        return;
    }

    if (mix_info.dst_mix_id == UnusedMixId) {
        if (mix_info.dst_splitter_id != UnusedSplitterId) {
            s16 dest_id{0};
            auto destination{
                splitter_context.GetDesintationData(mix_info.dst_splitter_id, dest_id)};
            while (destination != nullptr) {
                if (destination->IsConfigured()) {
                    auto splitter_mix_id{destination->GetMixId()};
                    if (splitter_mix_id < mix_context.GetCount()) {
                        auto splitter_mix_info{mix_context.GetInfo(splitter_mix_id)};
                        const s16 input_index{static_cast<s16>(mix_info.buffer_offset +
                                                               (dest_id % mix_info.buffer_count))};
                        for (s16 i = 0; i < splitter_mix_info->buffer_count; i++) {
                            auto volume{mix_info.volume * destination->GetMixVolume(i)};
                            if (volume != 0.0f) {
                                command_buffer.GenerateMixCommand(
                                    mix_info.node_id, input_index,
                                    splitter_mix_info->buffer_offset + i, mix_info.buffer_offset,
                                    volume, precision);
                            }
                        }
                    }
                }
                dest_id++;
                destination =
                    splitter_context.GetDesintationData(mix_info.dst_splitter_id, dest_id);
            }
        }
    } else {
        auto dest_mix_info{mix_context.GetInfo(mix_info.dst_mix_id)};
        for (s16 i = 0; i < mix_info.buffer_count; i++) {
            for (s16 j = 0; j < dest_mix_info->buffer_count; j++) {
                auto volume{mix_info.volume * mix_info.mix_volumes[i][j]};
                if (volume != 0.0f) {
                    command_buffer.GenerateMixCommand(mix_info.node_id, mix_info.buffer_offset + i,
                                                      dest_mix_info->buffer_offset + j,
                                                      mix_info.buffer_offset, volume, precision);
                }
            }
        }
    }
}

void CommandGenerator::GenerateSubMixCommand(MixInfo& mix_info) {
    command_buffer.GenerateDepopForMixBuffersCommand(mix_info.node_id, mix_info,
                                                     render_context.depop_buffer);
    GenerateEffectCommand(mix_info);

    DetailAspect mix_detail_aspect(*this, PerformanceEntryType::SubMix, mix_info.node_id,
                                   PerformanceDetailType::Unk5);

    GenerateMixCommands(mix_info);

    if (mix_detail_aspect.initialized) {
        command_buffer.GeneratePerformanceCommand(mix_detail_aspect.node_id, PerformanceState::Stop,
                                                  mix_detail_aspect.performance_entry_address);
    }
}

void CommandGenerator::GenerateSubMixCommands() {
    const auto submix_count{mix_context.GetCount()};
    for (s32 i = 0; i < submix_count; i++) {
        auto sorted_info{mix_context.GetSortedInfo(i)};
        if (!sorted_info->in_use || sorted_info->mix_id == FinalMixId) {
            continue;
        }

        EntryAspect submix_entry_aspect(*this, PerformanceEntryType::SubMix, sorted_info->node_id);

        GenerateSubMixCommand(*sorted_info);

        if (submix_entry_aspect.initialized) {
            command_buffer.GeneratePerformanceCommand(
                submix_entry_aspect.node_id, PerformanceState::Stop,
                submix_entry_aspect.performance_entry_address);
        }
    }
}

void CommandGenerator::GenerateFinalMixCommand() {
    auto& final_mix_info{*mix_context.GetFinalMixInfo()};

    command_buffer.GenerateDepopForMixBuffersCommand(final_mix_info.node_id, final_mix_info,
                                                     render_context.depop_buffer);
    GenerateEffectCommand(final_mix_info);

    u8 precision{15};
    if (render_context.behavior->IsVolumeMixParameterPrecisionQ23Supported()) {
        precision = 23;
    }

    for (s16 i = 0; i < final_mix_info.buffer_count; i++) {
        DetailAspect volume_aspect(*this, PerformanceEntryType::FinalMix, final_mix_info.node_id,
                                   PerformanceDetailType::Unk3);
        command_buffer.GenerateVolumeCommand(final_mix_info.node_id, final_mix_info.buffer_offset,
                                             i, final_mix_info.volume, precision);
        if (volume_aspect.initialized) {
            command_buffer.GeneratePerformanceCommand(volume_aspect.node_id, PerformanceState::Stop,
                                                      volume_aspect.performance_entry_address);
        }
    }
}

void CommandGenerator::GenerateFinalMixCommands() {
    auto final_mix_info{mix_context.GetFinalMixInfo()};
    EntryAspect final_mix_entry(*this, PerformanceEntryType::FinalMix, final_mix_info->node_id);
    GenerateFinalMixCommand();
    if (final_mix_entry.initialized) {
        command_buffer.GeneratePerformanceCommand(final_mix_entry.node_id, PerformanceState::Stop,
                                                  final_mix_entry.performance_entry_address);
    }
}

void CommandGenerator::GenerateSinkCommands() {
    const auto sink_count{sink_context.GetCount()};

    for (u32 i = 0; i < sink_count; i++) {
        auto sink_info{sink_context.GetInfo(i)};
        if (sink_info->IsUsed() && sink_info->GetType() == SinkInfoBase::Type::DeviceSink) {
            auto state{reinterpret_cast<DeviceSinkInfo::DeviceState*>(sink_info->GetState())};
            if (command_header.sample_rate != TargetSampleRate &&
                state->upsampler_info == nullptr) {
                auto device_state{sink_info->GetDeviceState()};
                device_state->upsampler_info = render_context.upsampler_manager->Allocate();
            }

            EntryAspect device_sink_entry(*this, PerformanceEntryType::Sink,
                                          sink_info->GetNodeId());
            auto final_mix{mix_context.GetFinalMixInfo()};
            GenerateSinkCommand(final_mix->buffer_offset, *sink_info);

            if (device_sink_entry.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    device_sink_entry.node_id, PerformanceState::Stop,
                    device_sink_entry.performance_entry_address);
            }
        }
    }

    for (u32 i = 0; i < sink_count; i++) {
        auto sink_info{sink_context.GetInfo(i)};
        if (sink_info->IsUsed() && sink_info->GetType() == SinkInfoBase::Type::CircularBufferSink) {
            EntryAspect circular_buffer_entry(*this, PerformanceEntryType::Sink,
                                              sink_info->GetNodeId());
            auto final_mix{mix_context.GetFinalMixInfo()};
            GenerateSinkCommand(final_mix->buffer_offset, *sink_info);

            if (circular_buffer_entry.initialized) {
                command_buffer.GeneratePerformanceCommand(
                    circular_buffer_entry.node_id, PerformanceState::Stop,
                    circular_buffer_entry.performance_entry_address);
            }
        }
    }
}

void CommandGenerator::GenerateSinkCommand(const s16 buffer_offset, SinkInfoBase& sink_info) {
    if (sink_info.ShouldSkip()) {
        return;
    }

    switch (sink_info.GetType()) {
    case SinkInfoBase::Type::DeviceSink:
        GenerateDeviceSinkCommand(buffer_offset, sink_info);
        break;

    case SinkInfoBase::Type::CircularBufferSink:
        command_buffer.GenerateCircularBufferSinkCommand(sink_info.GetNodeId(), sink_info,
                                                         buffer_offset);
        break;

    default:
        LOG_ERROR(Service_Audio, "Invalid sink type {}", static_cast<u32>(sink_info.GetType()));
        break;
    }

    sink_info.UpdateForCommandGeneration();
}

void CommandGenerator::GenerateDeviceSinkCommand(const s16 buffer_offset, SinkInfoBase& sink_info) {
    auto& parameter{
        *reinterpret_cast<DeviceSinkInfo::DeviceInParameter*>(sink_info.GetParameter())};
    auto state{*reinterpret_cast<DeviceSinkInfo::DeviceState*>(sink_info.GetState())};

    if (render_context.channels == 2 && parameter.downmix_enabled) {
        command_buffer.GenerateDownMix6chTo2chCommand(InvalidNodeId, parameter.inputs,
                                                      buffer_offset, parameter.downmix_coeff);
    }

    if (state.upsampler_info != nullptr) {
        command_buffer.GenerateUpsampleCommand(
            InvalidNodeId, buffer_offset, *state.upsampler_info, parameter.input_count,
            parameter.inputs, command_header.buffer_count, command_header.sample_count,
            command_header.sample_rate);
    }

    command_buffer.GenerateDeviceSinkCommand(InvalidNodeId, buffer_offset, sink_info,
                                             render_context.session_id,
                                             command_header.samples_buffer);
}

void CommandGenerator::GeneratePerformanceCommand(
    s32 node_id, PerformanceState state, const PerformanceEntryAddresses& entry_addresses) {
    command_buffer.GeneratePerformanceCommand(node_id, state, entry_addresses);
}

} // namespace AudioCore::AudioRenderer
