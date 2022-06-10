// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <numbers>

#include "audio_core/algorithm/interpolate.h"
#include "audio_core/command_generator.h"
#include "audio_core/effect_context.h"
#include "audio_core/mix_context.h"
#include "audio_core/voice_context.h"
#include "common/common_types.h"
#include "core/memory.h"

namespace AudioCore {
namespace {
constexpr std::size_t MIX_BUFFER_SIZE = 0x3f00;
constexpr std::size_t SCALED_MIX_BUFFER_SIZE = MIX_BUFFER_SIZE << 15ULL;
using DelayLineTimes = std::array<f32, AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT>;

constexpr DelayLineTimes FDN_MIN_DELAY_LINE_TIMES{5.0f, 6.0f, 13.0f, 14.0f};
constexpr DelayLineTimes FDN_MAX_DELAY_LINE_TIMES{45.704f, 82.782f, 149.94f, 271.58f};
constexpr DelayLineTimes DECAY0_MAX_DELAY_LINE_TIMES{17.0f, 13.0f, 9.0f, 7.0f};
constexpr DelayLineTimes DECAY1_MAX_DELAY_LINE_TIMES{19.0f, 11.0f, 10.0f, 6.0f};
constexpr std::array<f32, AudioCommon::I3DL2REVERB_TAPS> EARLY_TAP_TIMES{
    0.017136f, 0.059154f, 0.161733f, 0.390186f, 0.425262f, 0.455411f, 0.689737f,
    0.745910f, 0.833844f, 0.859502f, 0.000000f, 0.075024f, 0.168788f, 0.299901f,
    0.337443f, 0.371903f, 0.599011f, 0.716741f, 0.817859f, 0.851664f};
constexpr std::array<f32, AudioCommon::I3DL2REVERB_TAPS> EARLY_GAIN{
    0.67096f, 0.61027f, 1.0f,     0.35680f, 0.68361f, 0.65978f, 0.51939f,
    0.24712f, 0.45945f, 0.45021f, 0.64196f, 0.54879f, 0.92925f, 0.38270f,
    0.72867f, 0.69794f, 0.5464f,  0.24563f, 0.45214f, 0.44042f};

template <std::size_t N>
void ApplyMix(std::span<s32> output, std::span<const s32> input, s32 gain, s32 sample_count) {
    for (std::size_t i = 0; i < static_cast<std::size_t>(sample_count); i += N) {
        for (std::size_t j = 0; j < N; j++) {
            output[i + j] +=
                static_cast<s32>((static_cast<s64>(input[i + j]) * gain + 0x4000) >> 15);
        }
    }
}

s32 ApplyMixRamp(std::span<s32> output, std::span<const s32> input, float gain, float delta,
                 s32 sample_count) {
    // XC2 passes in NaN mix volumes, causing further issues as we handle everything as s32 rather
    // than float, so the NaN propogation is lost. As the samples get further modified for
    // volume etc, they can get out of NaN range, so a later heuristic for catching this is
    // more difficult. Handle it here by setting these samples to silence.
    if (std::isnan(gain)) {
        gain = 0.0f;
        delta = 0.0f;
    }

    s32 x = 0;
    for (s32 i = 0; i < sample_count; i++) {
        x = static_cast<s32>(static_cast<float>(input[i]) * gain);
        output[i] += x;
        gain += delta;
    }
    return x;
}

void ApplyGain(std::span<s32> output, std::span<const s32> input, s32 gain, s32 delta,
               s32 sample_count) {
    for (s32 i = 0; i < sample_count; i++) {
        output[i] = static_cast<s32>((static_cast<s64>(input[i]) * gain + 0x4000) >> 15);
        gain += delta;
    }
}

void ApplyGainWithoutDelta(std::span<s32> output, std::span<const s32> input, s32 gain,
                           s32 sample_count) {
    for (s32 i = 0; i < sample_count; i++) {
        output[i] = static_cast<s32>((static_cast<s64>(input[i]) * gain + 0x4000) >> 15);
    }
}

s32 ApplyMixDepop(std::span<s32> output, s32 first_sample, s32 delta, s32 sample_count) {
    const bool positive = first_sample > 0;
    auto final_sample = std::abs(first_sample);
    for (s32 i = 0; i < sample_count; i++) {
        final_sample = static_cast<s32>((static_cast<s64>(final_sample) * delta) >> 15);
        if (positive) {
            output[i] += final_sample;
        } else {
            output[i] -= final_sample;
        }
    }
    if (positive) {
        return final_sample;
    } else {
        return -final_sample;
    }
}

float Pow10(float x) {
    if (x >= 0.0f) {
        return 1.0f;
    } else if (x <= -5.3f) {
        return 0.0f;
    }
    return std::pow(10.0f, x);
}

float SinD(float degrees) {
    return std::sin(degrees * std::numbers::pi_v<float> / 180.0f);
}

float CosD(float degrees) {
    return std::cos(degrees * std::numbers::pi_v<float> / 180.0f);
}

float ToFloat(s32 sample) {
    return static_cast<float>(sample) / 65536.f;
}

s32 ToS32(float sample) {
    constexpr auto min = -8388608.0f;
    constexpr auto max = 8388607.f;
    float rescaled_sample = sample * 65536.0f;
    if (rescaled_sample < min) {
        rescaled_sample = min;
    }
    if (rescaled_sample > max) {
        rescaled_sample = max;
    }
    return static_cast<s32>(rescaled_sample);
}

constexpr std::array<u8, 20> REVERB_TAP_INDEX_1CH{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

constexpr std::array<u8, 20> REVERB_TAP_INDEX_2CH{0, 0, 0, 1, 1, 1, 1, 0, 0, 0,
                                                  1, 1, 1, 0, 0, 0, 0, 1, 1, 1};

constexpr std::array<u8, 20> REVERB_TAP_INDEX_4CH{0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                                  1, 1, 1, 0, 0, 0, 0, 3, 3, 3};

constexpr std::array<u8, 20> REVERB_TAP_INDEX_6CH{4, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                                  1, 1, 1, 0, 0, 0, 0, 3, 3, 3};

template <std::size_t CHANNEL_COUNT>
void ApplyReverbGeneric(
    I3dl2ReverbState& state,
    const std::array<std::span<const s32>, AudioCommon::MAX_CHANNEL_COUNT>& input,
    const std::array<std::span<s32>, AudioCommon::MAX_CHANNEL_COUNT>& output, s32 sample_count) {

    auto GetTapLookup = []() {
        if constexpr (CHANNEL_COUNT == 1) {
            return REVERB_TAP_INDEX_1CH;
        } else if constexpr (CHANNEL_COUNT == 2) {
            return REVERB_TAP_INDEX_2CH;
        } else if constexpr (CHANNEL_COUNT == 4) {
            return REVERB_TAP_INDEX_4CH;
        } else if constexpr (CHANNEL_COUNT == 6) {
            return REVERB_TAP_INDEX_6CH;
        }
    };

    const auto& tap_index_lut = GetTapLookup();
    for (s32 sample = 0; sample < sample_count; sample++) {
        std::array<f32, CHANNEL_COUNT> out_samples{};
        std::array<f32, AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT> fsamp{};
        std::array<f32, AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT> mixed{};
        std::array<f32, AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT> osamp{};

        // Mix everything into a single sample
        s32 temp_mixed_sample = 0;
        for (std::size_t i = 0; i < CHANNEL_COUNT; i++) {
            temp_mixed_sample += input[i][sample];
        }
        const auto current_sample = ToFloat(temp_mixed_sample);
        const auto early_tap = state.early_delay_line.TapOut(state.early_to_late_taps);

        for (std::size_t i = 0; i < AudioCommon::I3DL2REVERB_TAPS; i++) {
            const auto tapped_samp =
                state.early_delay_line.TapOut(state.early_tap_steps[i]) * EARLY_GAIN[i];
            out_samples[tap_index_lut[i]] += tapped_samp;

            if constexpr (CHANNEL_COUNT == 6) {
                // handle lfe
                out_samples[5] += tapped_samp;
            }
        }

        state.lowpass_0 = current_sample * state.lowpass_2 + state.lowpass_0 * state.lowpass_1;
        state.early_delay_line.Tick(state.lowpass_0);

        for (std::size_t i = 0; i < CHANNEL_COUNT; i++) {
            out_samples[i] *= state.early_gain;
        }

        // Two channel seems to apply a latet gain, we require to save this
        f32 filter{};
        for (std::size_t i = 0; i < AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT; i++) {
            filter = state.fdn_delay_line[i].GetOutputSample();
            const auto computed = filter * state.lpf_coefficients[0][i] + state.shelf_filter[i];
            state.shelf_filter[i] =
                filter * state.lpf_coefficients[1][i] + computed * state.lpf_coefficients[2][i];
            fsamp[i] = computed;
        }

        // Mixing matrix
        mixed[0] = fsamp[1] + fsamp[2];
        mixed[1] = -fsamp[0] - fsamp[3];
        mixed[2] = fsamp[0] - fsamp[3];
        mixed[3] = fsamp[1] - fsamp[2];

        if constexpr (CHANNEL_COUNT == 2) {
            for (auto& mix : mixed) {
                mix *= (filter * state.late_gain);
            }
        }

        for (std::size_t i = 0; i < AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT; i++) {
            const auto late = early_tap * state.late_gain;
            osamp[i] = state.decay_delay_line0[i].Tick(late + mixed[i]);
            osamp[i] = state.decay_delay_line1[i].Tick(osamp[i]);
            state.fdn_delay_line[i].Tick(osamp[i]);
        }

        if constexpr (CHANNEL_COUNT == 1) {
            output[0][sample] = ToS32(state.dry_gain * ToFloat(input[0][sample]) +
                                      (out_samples[0] + osamp[0] + osamp[1]));
        } else if constexpr (CHANNEL_COUNT == 2 || CHANNEL_COUNT == 4) {
            for (std::size_t i = 0; i < CHANNEL_COUNT; i++) {
                output[i][sample] =
                    ToS32(state.dry_gain * ToFloat(input[i][sample]) + (out_samples[i] + osamp[i]));
            }
        } else if constexpr (CHANNEL_COUNT == 6) {
            const auto temp_center = state.center_delay_line.Tick(0.5f * (osamp[2] - osamp[3]));
            for (std::size_t i = 0; i < 4; i++) {
                output[i][sample] =
                    ToS32(state.dry_gain * ToFloat(input[i][sample]) + (out_samples[i] + osamp[i]));
            }
            output[4][sample] =
                ToS32(state.dry_gain * ToFloat(input[4][sample]) + (out_samples[4] + temp_center));
            output[5][sample] =
                ToS32(state.dry_gain * ToFloat(input[5][sample]) + (out_samples[5] + osamp[3]));
        }
    }
}

} // namespace

CommandGenerator::CommandGenerator(AudioCommon::AudioRendererParameter& worker_params_,
                                   VoiceContext& voice_context_, MixContext& mix_context_,
                                   SplitterContext& splitter_context_,
                                   EffectContext& effect_context_, Core::Memory::Memory& memory_)
    : worker_params(worker_params_), voice_context(voice_context_), mix_context(mix_context_),
      splitter_context(splitter_context_), effect_context(effect_context_), memory(memory_),
      mix_buffer((worker_params.mix_buffer_count + AudioCommon::MAX_CHANNEL_COUNT) *
                 worker_params.sample_count),
      sample_buffer(MIX_BUFFER_SIZE),
      depop_buffer((worker_params.mix_buffer_count + AudioCommon::MAX_CHANNEL_COUNT) *
                   worker_params.sample_count) {}
CommandGenerator::~CommandGenerator() = default;

void CommandGenerator::ClearMixBuffers() {
    std::fill(mix_buffer.begin(), mix_buffer.end(), 0);
    std::fill(sample_buffer.begin(), sample_buffer.end(), 0);
    // std::fill(depop_buffer.begin(), depop_buffer.end(), 0);
}

void CommandGenerator::GenerateVoiceCommands() {
    if (dumping_frame) {
        LOG_DEBUG(Audio, "(DSP_TRACE) GenerateVoiceCommands");
    }
    // Grab all our voices
    const auto voice_count = voice_context.GetVoiceCount();
    for (std::size_t i = 0; i < voice_count; i++) {
        auto& voice_info = voice_context.GetSortedInfo(i);
        // Update voices and check if we should queue them
        if (voice_info.ShouldSkip() || !voice_info.UpdateForCommandGeneration(voice_context)) {
            continue;
        }

        // Queue our voice
        GenerateVoiceCommand(voice_info);
    }
    // Update our splitters
    splitter_context.UpdateInternalState();
}

void CommandGenerator::GenerateVoiceCommand(ServerVoiceInfo& voice_info) {
    auto& in_params = voice_info.GetInParams();
    const auto channel_count = in_params.channel_count;

    for (s32 channel = 0; channel < channel_count; channel++) {
        const auto resource_id = in_params.voice_channel_resource_id[channel];
        auto& dsp_state = voice_context.GetDspSharedState(resource_id);
        auto& channel_resource = voice_context.GetChannelResource(resource_id);

        // Decode our samples for our channel
        GenerateDataSourceCommand(voice_info, dsp_state, channel);

        if (in_params.should_depop) {
            in_params.last_volume = 0.0f;
        } else if (in_params.splitter_info_id != AudioCommon::NO_SPLITTER ||
                   in_params.mix_id != AudioCommon::NO_MIX) {
            // Apply a biquad filter if needed
            GenerateBiquadFilterCommandForVoice(voice_info, dsp_state,
                                                worker_params.mix_buffer_count, channel);
            // Base voice volume ramping
            GenerateVolumeRampCommand(in_params.last_volume, in_params.volume, channel,
                                      in_params.node_id);
            in_params.last_volume = in_params.volume;

            if (in_params.mix_id != AudioCommon::NO_MIX) {
                // If we're using a mix id
                auto& mix_info = mix_context.GetInfo(in_params.mix_id);
                const auto& dest_mix_params = mix_info.GetInParams();

                // Voice Mixing
                GenerateVoiceMixCommand(
                    channel_resource.GetCurrentMixVolume(), channel_resource.GetLastMixVolume(),
                    dsp_state, dest_mix_params.buffer_offset, dest_mix_params.buffer_count,
                    worker_params.mix_buffer_count + channel, in_params.node_id);

                // Update last mix volumes
                channel_resource.UpdateLastMixVolumes();
            } else if (in_params.splitter_info_id != AudioCommon::NO_SPLITTER) {
                s32 base = channel;
                while (auto* destination_data =
                           GetDestinationData(in_params.splitter_info_id, base)) {
                    base += channel_count;

                    if (!destination_data->IsConfigured()) {
                        continue;
                    }
                    if (destination_data->GetMixId() >= static_cast<int>(mix_context.GetCount())) {
                        continue;
                    }

                    const auto& mix_info = mix_context.GetInfo(destination_data->GetMixId());
                    const auto& dest_mix_params = mix_info.GetInParams();
                    GenerateVoiceMixCommand(
                        destination_data->CurrentMixVolumes(), destination_data->LastMixVolumes(),
                        dsp_state, dest_mix_params.buffer_offset, dest_mix_params.buffer_count,
                        worker_params.mix_buffer_count + channel, in_params.node_id);
                    destination_data->MarkDirty();
                }
            }
            // Update biquad filter enabled states
            for (std::size_t i = 0; i < AudioCommon::MAX_BIQUAD_FILTERS; i++) {
                in_params.was_biquad_filter_enabled[i] = in_params.biquad_filter[i].enabled;
            }
        }
    }
}

void CommandGenerator::GenerateSubMixCommands() {
    const auto mix_count = mix_context.GetCount();
    for (std::size_t i = 0; i < mix_count; i++) {
        auto& mix_info = mix_context.GetSortedInfo(i);
        const auto& in_params = mix_info.GetInParams();
        if (!in_params.in_use || in_params.mix_id == AudioCommon::FINAL_MIX) {
            continue;
        }
        GenerateSubMixCommand(mix_info);
    }
}

void CommandGenerator::GenerateFinalMixCommands() {
    GenerateFinalMixCommand();
}

void CommandGenerator::PreCommand() {
    if (!dumping_frame) {
        return;
    }
    for (std::size_t i = 0; i < splitter_context.GetInfoCount(); i++) {
        const auto& base = splitter_context.GetInfo(i);
        std::string graph = fmt::format("b[{}]", i);
        const auto* head = base.GetHead();
        while (head != nullptr) {
            graph += fmt::format("->{}", head->GetMixId());
            head = head->GetNextDestination();
        }
        LOG_DEBUG(Audio, "(DSP_TRACE) SplitterGraph splitter_info={}, {}", i, graph);
    }
}

void CommandGenerator::PostCommand() {
    if (!dumping_frame) {
        return;
    }
    dumping_frame = false;
}

void CommandGenerator::GenerateDataSourceCommand(ServerVoiceInfo& voice_info, VoiceState& dsp_state,
                                                 s32 channel) {
    const auto& in_params = voice_info.GetInParams();
    const auto depop = in_params.should_depop;

    if (depop) {
        if (in_params.mix_id != AudioCommon::NO_MIX) {
            auto& mix_info = mix_context.GetInfo(in_params.mix_id);
            const auto& mix_in = mix_info.GetInParams();
            GenerateDepopPrepareCommand(dsp_state, mix_in.buffer_count, mix_in.buffer_offset);
        } else if (in_params.splitter_info_id != AudioCommon::NO_SPLITTER) {
            s32 index{};
            while (const auto* destination =
                       GetDestinationData(in_params.splitter_info_id, index++)) {
                if (!destination->IsConfigured()) {
                    continue;
                }
                auto& mix_info = mix_context.GetInfo(destination->GetMixId());
                const auto& mix_in = mix_info.GetInParams();
                GenerateDepopPrepareCommand(dsp_state, mix_in.buffer_count, mix_in.buffer_offset);
            }
        }
    } else {
        switch (in_params.sample_format) {
        case SampleFormat::Pcm8:
        case SampleFormat::Pcm16:
        case SampleFormat::Pcm32:
        case SampleFormat::PcmFloat:
            DecodeFromWaveBuffers(voice_info, GetChannelMixBuffer(channel), dsp_state, channel,
                                  worker_params.sample_rate, worker_params.sample_count,
                                  in_params.node_id);
            break;
        case SampleFormat::Adpcm:
            ASSERT(channel == 0 && in_params.channel_count == 1);
            DecodeFromWaveBuffers(voice_info, GetChannelMixBuffer(0), dsp_state, 0,
                                  worker_params.sample_rate, worker_params.sample_count,
                                  in_params.node_id);
            break;
        default:
            ASSERT_MSG(false, "Unimplemented sample format={}", in_params.sample_format);
        }
    }
}

void CommandGenerator::GenerateBiquadFilterCommandForVoice(ServerVoiceInfo& voice_info,
                                                           VoiceState& dsp_state,
                                                           [[maybe_unused]] s32 mix_buffer_count,
                                                           [[maybe_unused]] s32 channel) {
    for (std::size_t i = 0; i < AudioCommon::MAX_BIQUAD_FILTERS; i++) {
        const auto& in_params = voice_info.GetInParams();
        auto& biquad_filter = in_params.biquad_filter[i];
        // Check if biquad filter is actually used
        if (!biquad_filter.enabled) {
            continue;
        }

        // Reinitialize our biquad filter state if it was enabled previously
        if (!in_params.was_biquad_filter_enabled[i]) {
            dsp_state.biquad_filter_state.fill(0);
        }

        // Generate biquad filter
        // GenerateBiquadFilterCommand(mix_buffer_count, biquad_filter,
        // dsp_state.biquad_filter_state,
        //                            mix_buffer_count + channel, mix_buffer_count + channel,
        //                            worker_params.sample_count, voice_info.GetInParams().node_id);
    }
}

void CommandGenerator::GenerateBiquadFilterCommand([[maybe_unused]] s32 mix_buffer_id,
                                                   const BiquadFilterParameter& params,
                                                   std::array<s64, 2>& state,
                                                   std::size_t input_offset,
                                                   std::size_t output_offset, s32 sample_count,
                                                   s32 node_id) {
    if (dumping_frame) {
        LOG_DEBUG(Audio,
                  "(DSP_TRACE) GenerateBiquadFilterCommand node_id={}, "
                  "input_mix_buffer={}, output_mix_buffer={}",
                  node_id, input_offset, output_offset);
    }
    std::span<const s32> input = GetMixBuffer(input_offset);
    std::span<s32> output = GetMixBuffer(output_offset);

    // Biquad filter parameters
    const auto [n0, n1, n2] = params.numerator;
    const auto [d0, d1] = params.denominator;

    // Biquad filter states
    auto [s0, s1] = state;

    constexpr s64 int32_min = std::numeric_limits<s32>::min();
    constexpr s64 int32_max = std::numeric_limits<s32>::max();

    for (int i = 0; i < sample_count; ++i) {
        const auto sample = static_cast<s64>(input[i]);
        const auto f = (sample * n0 + s0 + 0x4000) >> 15;
        const auto y = std::clamp(f, int32_min, int32_max);
        s0 = sample * n1 + y * d0 + s1;
        s1 = sample * n2 + y * d1;
        output[i] = static_cast<s32>(y);
    }

    state = {s0, s1};
}

void CommandGenerator::GenerateDepopPrepareCommand(VoiceState& dsp_state,
                                                   std::size_t mix_buffer_count,
                                                   std::size_t mix_buffer_offset) {
    for (std::size_t i = 0; i < mix_buffer_count; i++) {
        auto& sample = dsp_state.previous_samples[i];
        if (sample != 0) {
            depop_buffer[mix_buffer_offset + i] += sample;
            sample = 0;
        }
    }
}

void CommandGenerator::GenerateDepopForMixBuffersCommand(std::size_t mix_buffer_count,
                                                         std::size_t mix_buffer_offset,
                                                         s32 sample_rate) {
    const std::size_t end_offset =
        std::min(mix_buffer_offset + mix_buffer_count, GetTotalMixBufferCount());
    const s32 delta = sample_rate == 48000 ? 0x7B29 : 0x78CB;
    for (std::size_t i = mix_buffer_offset; i < end_offset; i++) {
        if (depop_buffer[i] == 0) {
            continue;
        }

        depop_buffer[i] =
            ApplyMixDepop(GetMixBuffer(i), depop_buffer[i], delta, worker_params.sample_count);
    }
}

void CommandGenerator::GenerateEffectCommand(ServerMixInfo& mix_info) {
    const std::size_t effect_count = effect_context.GetCount();
    const auto buffer_offset = mix_info.GetInParams().buffer_offset;
    for (std::size_t i = 0; i < effect_count; i++) {
        const auto index = mix_info.GetEffectOrder(i);
        if (index == AudioCommon::NO_EFFECT_ORDER) {
            break;
        }
        auto* info = effect_context.GetInfo(index);
        const auto type = info->GetType();

        // TODO(ogniK): Finish remaining effects
        switch (type) {
        case EffectType::Aux:
            GenerateAuxCommand(buffer_offset, info, info->IsEnabled());
            break;
        case EffectType::I3dl2Reverb:
            GenerateI3dl2ReverbEffectCommand(buffer_offset, info, info->IsEnabled());
            break;
        case EffectType::BiquadFilter:
            GenerateBiquadFilterEffectCommand(buffer_offset, info, info->IsEnabled());
            break;
        default:
            break;
        }

        info->UpdateForCommandGeneration();
    }
}

void CommandGenerator::GenerateI3dl2ReverbEffectCommand(s32 mix_buffer_offset, EffectBase* info,
                                                        bool enabled) {
    auto* reverb = dynamic_cast<EffectI3dl2Reverb*>(info);
    const auto& params = reverb->GetParams();
    auto& state = reverb->GetState();
    const auto channel_count = params.channel_count;

    if (channel_count != 1 && channel_count != 2 && channel_count != 4 && channel_count != 6) {
        return;
    }

    std::array<std::span<const s32>, AudioCommon::MAX_CHANNEL_COUNT> input{};
    std::array<std::span<s32>, AudioCommon::MAX_CHANNEL_COUNT> output{};

    const auto status = params.status;
    for (s32 i = 0; i < channel_count; i++) {
        input[i] = GetMixBuffer(mix_buffer_offset + params.input[i]);
        output[i] = GetMixBuffer(mix_buffer_offset + params.output[i]);
    }

    if (enabled) {
        if (status == ParameterStatus::Initialized) {
            InitializeI3dl2Reverb(reverb->GetParams(), state, info->GetWorkBuffer());
        } else if (status == ParameterStatus::Updating) {
            UpdateI3dl2Reverb(reverb->GetParams(), state, false);
        }
    }

    if (enabled) {
        switch (channel_count) {
        case 1:
            ApplyReverbGeneric<1>(state, input, output, worker_params.sample_count);
            break;
        case 2:
            ApplyReverbGeneric<2>(state, input, output, worker_params.sample_count);
            break;
        case 4:
            ApplyReverbGeneric<4>(state, input, output, worker_params.sample_count);
            break;
        case 6:
            ApplyReverbGeneric<6>(state, input, output, worker_params.sample_count);
            break;
        }
    } else {
        for (s32 i = 0; i < channel_count; i++) {
            // Only copy if the buffer input and output do not match!
            if ((mix_buffer_offset + params.input[i]) != (mix_buffer_offset + params.output[i])) {
                std::memcpy(output[i].data(), input[i].data(),
                            worker_params.sample_count * sizeof(s32));
            }
        }
    }
}

void CommandGenerator::GenerateBiquadFilterEffectCommand(s32 mix_buffer_offset, EffectBase* info,
                                                         bool enabled) {
    if (!enabled) {
        return;
    }
    const auto& params = dynamic_cast<EffectBiquadFilter*>(info)->GetParams();
    const auto channel_count = params.channel_count;
    for (s32 i = 0; i < channel_count; i++) {
        // TODO(ogniK): Actually implement biquad filter
        if (params.input[i] != params.output[i]) {
            std::span<const s32> input = GetMixBuffer(mix_buffer_offset + params.input[i]);
            std::span<s32> output = GetMixBuffer(mix_buffer_offset + params.output[i]);
            ApplyMix<1>(output, input, 32768, worker_params.sample_count);
        }
    }
}

void CommandGenerator::GenerateAuxCommand(s32 mix_buffer_offset, EffectBase* info, bool enabled) {
    auto* aux = dynamic_cast<EffectAuxInfo*>(info);
    const auto& params = aux->GetParams();
    if (aux->GetSendBuffer() != 0 && aux->GetRecvBuffer() != 0) {
        const auto max_channels = params.count;
        u32 offset{};
        for (u32 channel = 0; channel < max_channels; channel++) {
            u32 write_count = 0;
            if (channel == (max_channels - 1)) {
                write_count = offset + worker_params.sample_count;
            }

            const auto input_index = params.input_mix_buffers[channel] + mix_buffer_offset;
            const auto output_index = params.output_mix_buffers[channel] + mix_buffer_offset;

            if (enabled) {
                AuxInfoDSP send_info{};
                AuxInfoDSP recv_info{};
                memory.ReadBlock(aux->GetSendInfo(), &send_info, sizeof(AuxInfoDSP));
                memory.ReadBlock(aux->GetRecvInfo(), &recv_info, sizeof(AuxInfoDSP));

                WriteAuxBuffer(send_info, aux->GetSendBuffer(), params.sample_count,
                               GetMixBuffer(input_index), worker_params.sample_count, offset,
                               write_count);
                memory.WriteBlock(aux->GetSendInfo(), &send_info, sizeof(AuxInfoDSP));

                const auto samples_read = ReadAuxBuffer(
                    recv_info, aux->GetRecvBuffer(), params.sample_count,
                    GetMixBuffer(output_index), worker_params.sample_count, offset, write_count);
                memory.WriteBlock(aux->GetRecvInfo(), &recv_info, sizeof(AuxInfoDSP));

                if (samples_read != static_cast<int>(worker_params.sample_count) &&
                    samples_read <= params.sample_count) {
                    std::memset(GetMixBuffer(output_index).data(), 0,
                                params.sample_count - samples_read);
                }
            } else {
                AuxInfoDSP empty{};
                memory.WriteBlock(aux->GetSendInfo(), &empty, sizeof(AuxInfoDSP));
                memory.WriteBlock(aux->GetRecvInfo(), &empty, sizeof(AuxInfoDSP));
                if (output_index != input_index) {
                    std::memcpy(GetMixBuffer(output_index).data(), GetMixBuffer(input_index).data(),
                                worker_params.sample_count * sizeof(s32));
                }
            }

            offset += worker_params.sample_count;
        }
    }
}

ServerSplitterDestinationData* CommandGenerator::GetDestinationData(s32 splitter_id, s32 index) {
    if (splitter_id == AudioCommon::NO_SPLITTER) {
        return nullptr;
    }
    return splitter_context.GetDestinationData(splitter_id, index);
}

s32 CommandGenerator::WriteAuxBuffer(AuxInfoDSP& dsp_info, VAddr send_buffer, u32 max_samples,
                                     std::span<const s32> data, u32 sample_count, u32 write_offset,
                                     u32 write_count) {
    if (max_samples == 0) {
        return 0;
    }
    u32 offset = dsp_info.write_offset + write_offset;
    if (send_buffer == 0 || offset > max_samples) {
        return 0;
    }

    s32 data_offset{};
    u32 remaining = sample_count;
    while (remaining > 0) {
        // Get position in buffer
        const auto base = send_buffer + (offset * sizeof(u32));
        const auto samples_to_grab = std::min(max_samples - offset, remaining);
        // Write to output
        memory.WriteBlock(base, (data.data() + data_offset), samples_to_grab * sizeof(u32));
        offset = (offset + samples_to_grab) % max_samples;
        remaining -= samples_to_grab;
        data_offset += samples_to_grab;
    }

    if (write_count != 0) {
        dsp_info.write_offset = (dsp_info.write_offset + write_count) % max_samples;
    }
    return sample_count;
}

s32 CommandGenerator::ReadAuxBuffer(AuxInfoDSP& recv_info, VAddr recv_buffer, u32 max_samples,
                                    std::span<s32> out_data, u32 sample_count, u32 read_offset,
                                    u32 read_count) {
    if (max_samples == 0) {
        return 0;
    }

    u32 offset = recv_info.read_offset + read_offset;
    if (recv_buffer == 0 || offset > max_samples) {
        return 0;
    }

    u32 remaining = sample_count;
    s32 data_offset{};
    while (remaining > 0) {
        const auto base = recv_buffer + (offset * sizeof(u32));
        const auto samples_to_grab = std::min(max_samples - offset, remaining);
        std::vector<s32> buffer(samples_to_grab);
        memory.ReadBlock(base, buffer.data(), buffer.size() * sizeof(u32));
        std::memcpy(out_data.data() + data_offset, buffer.data(), buffer.size() * sizeof(u32));
        offset = (offset + samples_to_grab) % max_samples;
        remaining -= samples_to_grab;
        data_offset += samples_to_grab;
    }

    if (read_count != 0) {
        recv_info.read_offset = (recv_info.read_offset + read_count) % max_samples;
    }
    return sample_count;
}

void CommandGenerator::InitializeI3dl2Reverb(I3dl2ReverbParams& info, I3dl2ReverbState& state,
                                             std::vector<u8>& work_buffer) {
    // Reset state
    state.lowpass_0 = 0.0f;
    state.lowpass_1 = 0.0f;
    state.lowpass_2 = 0.0f;

    state.early_delay_line.Reset();
    state.early_tap_steps.fill(0);
    state.early_gain = 0.0f;
    state.late_gain = 0.0f;
    state.early_to_late_taps = 0;
    for (std::size_t i = 0; i < AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT; i++) {
        state.fdn_delay_line[i].Reset();
        state.decay_delay_line0[i].Reset();
        state.decay_delay_line1[i].Reset();
    }
    state.last_reverb_echo = 0.0f;
    state.center_delay_line.Reset();
    for (auto& coef : state.lpf_coefficients) {
        coef.fill(0.0f);
    }
    state.shelf_filter.fill(0.0f);
    state.dry_gain = 0.0f;

    const auto sample_rate = info.sample_rate / 1000;
    f32* work_buffer_ptr = reinterpret_cast<f32*>(work_buffer.data());

    s32 delay_samples{};
    for (std::size_t i = 0; i < AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT; i++) {
        delay_samples =
            AudioCommon::CalculateDelaySamples(sample_rate, FDN_MAX_DELAY_LINE_TIMES[i]);
        state.fdn_delay_line[i].Initialize(delay_samples, work_buffer_ptr);
        work_buffer_ptr += delay_samples + 1;

        delay_samples =
            AudioCommon::CalculateDelaySamples(sample_rate, DECAY0_MAX_DELAY_LINE_TIMES[i]);
        state.decay_delay_line0[i].Initialize(delay_samples, 0.0f, work_buffer_ptr);
        work_buffer_ptr += delay_samples + 1;

        delay_samples =
            AudioCommon::CalculateDelaySamples(sample_rate, DECAY1_MAX_DELAY_LINE_TIMES[i]);
        state.decay_delay_line1[i].Initialize(delay_samples, 0.0f, work_buffer_ptr);
        work_buffer_ptr += delay_samples + 1;
    }
    delay_samples = AudioCommon::CalculateDelaySamples(sample_rate, 5.0f);
    state.center_delay_line.Initialize(delay_samples, work_buffer_ptr);
    work_buffer_ptr += delay_samples + 1;

    delay_samples = AudioCommon::CalculateDelaySamples(sample_rate, 400.0f);
    state.early_delay_line.Initialize(delay_samples, work_buffer_ptr);

    UpdateI3dl2Reverb(info, state, true);
}

void CommandGenerator::UpdateI3dl2Reverb(I3dl2ReverbParams& info, I3dl2ReverbState& state,
                                         bool should_clear) {

    state.dry_gain = info.dry_gain;
    state.shelf_filter.fill(0.0f);
    state.lowpass_0 = 0.0f;
    state.early_gain = Pow10(std::min(info.room + info.reflection, 5000.0f) / 2000.0f);
    state.late_gain = Pow10(std::min(info.room + info.reverb, 5000.0f) / 2000.0f);

    const auto sample_rate = info.sample_rate / 1000;
    const f32 hf_gain = Pow10(info.room_hf / 2000.0f);
    if (hf_gain >= 1.0f) {
        state.lowpass_2 = 1.0f;
        state.lowpass_1 = 0.0f;
    } else {
        const auto a = 1.0f - hf_gain;
        const auto b = 2.0f * (2.0f - hf_gain * CosD(256.0f * info.hf_reference /
                                                     static_cast<f32>(info.sample_rate)));
        const auto c = std::sqrt(b * b - 4.0f * a * a);

        state.lowpass_1 = (b - c) / (2.0f * a);
        state.lowpass_2 = 1.0f - state.lowpass_1;
    }
    state.early_to_late_taps = AudioCommon::CalculateDelaySamples(
        sample_rate, 1000.0f * (info.reflection_delay + info.reverb_delay));

    state.last_reverb_echo = 0.6f * info.diffusion * 0.01f;
    for (std::size_t i = 0; i < AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT; i++) {
        const auto length =
            FDN_MIN_DELAY_LINE_TIMES[i] +
            (info.density / 100.0f) * (FDN_MAX_DELAY_LINE_TIMES[i] - FDN_MIN_DELAY_LINE_TIMES[i]);
        state.fdn_delay_line[i].SetDelay(AudioCommon::CalculateDelaySamples(sample_rate, length));

        const auto delay_sample_counts = state.fdn_delay_line[i].GetDelay() +
                                         state.decay_delay_line0[i].GetDelay() +
                                         state.decay_delay_line1[i].GetDelay();

        float a = (-60.0f * static_cast<f32>(delay_sample_counts)) /
                  (info.decay_time * static_cast<f32>(info.sample_rate));
        float b = a / info.hf_decay_ratio;
        float c = CosD(128.0f * 0.5f * info.hf_reference / static_cast<f32>(info.sample_rate)) /
                  SinD(128.0f * 0.5f * info.hf_reference / static_cast<f32>(info.sample_rate));
        float d = Pow10((b - a) / 40.0f);
        float e = Pow10((b + a) / 40.0f) * 0.7071f;

        state.lpf_coefficients[0][i] = e * ((d * c) + 1.0f) / (c + d);
        state.lpf_coefficients[1][i] = e * (1.0f - (d * c)) / (c + d);
        state.lpf_coefficients[2][i] = (c - d) / (c + d);

        state.decay_delay_line0[i].SetCoefficient(state.last_reverb_echo);
        state.decay_delay_line1[i].SetCoefficient(-0.9f * state.last_reverb_echo);
    }

    if (should_clear) {
        for (std::size_t i = 0; i < AudioCommon::I3DL2REVERB_DELAY_LINE_COUNT; i++) {
            state.fdn_delay_line[i].Clear();
            state.decay_delay_line0[i].Clear();
            state.decay_delay_line1[i].Clear();
        }
        state.early_delay_line.Clear();
        state.center_delay_line.Clear();
    }

    const auto max_early_delay = state.early_delay_line.GetMaxDelay();
    const auto reflection_time = 1000.0f * (0.9998f * info.reverb_delay + 0.02f);
    for (std::size_t tap = 0; tap < AudioCommon::I3DL2REVERB_TAPS; tap++) {
        const auto length = AudioCommon::CalculateDelaySamples(
            sample_rate, 1000.0f * info.reflection_delay + reflection_time * EARLY_TAP_TIMES[tap]);
        state.early_tap_steps[tap] = std::min(length, max_early_delay);
    }
}

void CommandGenerator::GenerateVolumeRampCommand(float last_volume, float current_volume,
                                                 s32 channel, s32 node_id) {
    const auto last = static_cast<s32>(last_volume * 32768.0f);
    const auto current = static_cast<s32>(current_volume * 32768.0f);
    const auto delta = static_cast<s32>((static_cast<float>(current) - static_cast<float>(last)) /
                                        static_cast<float>(worker_params.sample_count));

    if (dumping_frame) {
        LOG_DEBUG(Audio,
                  "(DSP_TRACE) GenerateVolumeRampCommand node_id={}, input={}, output={}, "
                  "last_volume={}, current_volume={}",
                  node_id, GetMixChannelBufferOffset(channel), GetMixChannelBufferOffset(channel),
                  last_volume, current_volume);
    }
    // Apply generic gain on samples
    ApplyGain(GetChannelMixBuffer(channel), GetChannelMixBuffer(channel), last, delta,
              worker_params.sample_count);
}

void CommandGenerator::GenerateVoiceMixCommand(const MixVolumeBuffer& mix_volumes,
                                               const MixVolumeBuffer& last_mix_volumes,
                                               VoiceState& dsp_state, s32 mix_buffer_offset,
                                               s32 mix_buffer_count, s32 voice_index, s32 node_id) {
    // Loop all our mix buffers
    for (s32 i = 0; i < mix_buffer_count; i++) {
        if (last_mix_volumes[i] != 0.0f || mix_volumes[i] != 0.0f) {
            const auto delta = static_cast<float>((mix_volumes[i] - last_mix_volumes[i])) /
                               static_cast<float>(worker_params.sample_count);

            if (dumping_frame) {
                LOG_DEBUG(Audio,
                          "(DSP_TRACE) GenerateVoiceMixCommand node_id={}, input={}, "
                          "output={}, last_volume={}, current_volume={}",
                          node_id, voice_index, mix_buffer_offset + i, last_mix_volumes[i],
                          mix_volumes[i]);
            }

            dsp_state.previous_samples[i] =
                ApplyMixRamp(GetMixBuffer(mix_buffer_offset + i), GetMixBuffer(voice_index),
                             last_mix_volumes[i], delta, worker_params.sample_count);
        } else {
            dsp_state.previous_samples[i] = 0;
        }
    }
}

void CommandGenerator::GenerateSubMixCommand(ServerMixInfo& mix_info) {
    if (dumping_frame) {
        LOG_DEBUG(Audio, "(DSP_TRACE) GenerateSubMixCommand");
    }
    const auto& in_params = mix_info.GetInParams();
    GenerateDepopForMixBuffersCommand(in_params.buffer_count, in_params.buffer_offset,
                                      in_params.sample_rate);

    GenerateEffectCommand(mix_info);

    GenerateMixCommands(mix_info);
}

void CommandGenerator::GenerateMixCommands(ServerMixInfo& mix_info) {
    if (!mix_info.HasAnyConnection()) {
        return;
    }
    const auto& in_params = mix_info.GetInParams();
    if (in_params.dest_mix_id != AudioCommon::NO_MIX) {
        const auto& dest_mix = mix_context.GetInfo(in_params.dest_mix_id);
        const auto& dest_in_params = dest_mix.GetInParams();

        const auto buffer_count = in_params.buffer_count;

        for (s32 i = 0; i < buffer_count; i++) {
            for (s32 j = 0; j < dest_in_params.buffer_count; j++) {
                const auto mixed_volume = in_params.volume * in_params.mix_volume[i][j];
                if (mixed_volume != 0.0f) {
                    GenerateMixCommand(dest_in_params.buffer_offset + j,
                                       in_params.buffer_offset + i, mixed_volume,
                                       in_params.node_id);
                }
            }
        }
    } else if (in_params.splitter_id != AudioCommon::NO_SPLITTER) {
        s32 base{};
        while (const auto* destination_data = GetDestinationData(in_params.splitter_id, base++)) {
            if (!destination_data->IsConfigured()) {
                continue;
            }

            const auto& dest_mix = mix_context.GetInfo(destination_data->GetMixId());
            const auto& dest_in_params = dest_mix.GetInParams();
            const auto mix_index = (base - 1) % in_params.buffer_count + in_params.buffer_offset;
            for (std::size_t i = 0; i < static_cast<std::size_t>(dest_in_params.buffer_count);
                 i++) {
                const auto mixed_volume = in_params.volume * destination_data->GetMixVolume(i);
                if (mixed_volume != 0.0f) {
                    GenerateMixCommand(dest_in_params.buffer_offset + i, mix_index, mixed_volume,
                                       in_params.node_id);
                }
            }
        }
    }
}

void CommandGenerator::GenerateMixCommand(std::size_t output_offset, std::size_t input_offset,
                                          float volume, s32 node_id) {

    if (dumping_frame) {
        LOG_DEBUG(Audio,
                  "(DSP_TRACE) GenerateMixCommand node_id={}, input={}, output={}, volume={}",
                  node_id, input_offset, output_offset, volume);
    }

    std::span<s32> output = GetMixBuffer(output_offset);
    std::span<const s32> input = GetMixBuffer(input_offset);

    const s32 gain = static_cast<s32>(volume * 32768.0f);
    // Mix with loop unrolling
    if (worker_params.sample_count % 4 == 0) {
        ApplyMix<4>(output, input, gain, worker_params.sample_count);
    } else if (worker_params.sample_count % 2 == 0) {
        ApplyMix<2>(output, input, gain, worker_params.sample_count);
    } else {
        ApplyMix<1>(output, input, gain, worker_params.sample_count);
    }
}

void CommandGenerator::GenerateFinalMixCommand() {
    if (dumping_frame) {
        LOG_DEBUG(Audio, "(DSP_TRACE) GenerateFinalMixCommand");
    }
    auto& mix_info = mix_context.GetFinalMixInfo();
    const auto& in_params = mix_info.GetInParams();

    GenerateDepopForMixBuffersCommand(in_params.buffer_count, in_params.buffer_offset,
                                      in_params.sample_rate);

    GenerateEffectCommand(mix_info);

    for (s32 i = 0; i < in_params.buffer_count; i++) {
        const s32 gain = static_cast<s32>(in_params.volume * 32768.0f);
        if (dumping_frame) {
            LOG_DEBUG(
                Audio,
                "(DSP_TRACE) ApplyGainWithoutDelta node_id={}, input={}, output={}, volume={}",
                in_params.node_id, in_params.buffer_offset + i, in_params.buffer_offset + i,
                in_params.volume);
        }
        ApplyGainWithoutDelta(GetMixBuffer(in_params.buffer_offset + i),
                              GetMixBuffer(in_params.buffer_offset + i), gain,
                              worker_params.sample_count);
    }
}

template <typename T>
s32 CommandGenerator::DecodePcm(ServerVoiceInfo& voice_info, VoiceState& dsp_state,
                                s32 sample_start_offset, s32 sample_end_offset, s32 sample_count,
                                s32 channel, std::size_t mix_offset) {
    const auto& in_params = voice_info.GetInParams();
    const auto& wave_buffer = in_params.wave_buffer[dsp_state.wave_buffer_index];
    if (wave_buffer.buffer_address == 0) {
        return 0;
    }
    if (wave_buffer.buffer_size == 0) {
        return 0;
    }
    if (sample_end_offset < sample_start_offset) {
        return 0;
    }
    const auto samples_remaining = (sample_end_offset - sample_start_offset) - dsp_state.offset;
    const auto start_offset =
        ((dsp_state.offset + sample_start_offset) * in_params.channel_count) * sizeof(T);
    const auto buffer_pos = wave_buffer.buffer_address + start_offset;
    const auto samples_processed = std::min(sample_count, samples_remaining);

    const auto channel_count = in_params.channel_count;
    std::vector<T> buffer(samples_processed * channel_count);
    memory.ReadBlock(buffer_pos, buffer.data(), buffer.size() * sizeof(T));

    if constexpr (std::is_floating_point_v<T>) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(samples_processed); i++) {
            sample_buffer[mix_offset + i] = static_cast<s32>(buffer[i * channel_count + channel] *
                                                             std::numeric_limits<s16>::max());
        }
    } else if constexpr (sizeof(T) == 1) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(samples_processed); i++) {
            sample_buffer[mix_offset + i] =
                static_cast<s32>(static_cast<f32>(buffer[i * channel_count + channel] /
                                                  std::numeric_limits<s8>::max()) *
                                 std::numeric_limits<s16>::max());
        }
    } else if constexpr (sizeof(T) == 2) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(samples_processed); i++) {
            sample_buffer[mix_offset + i] = buffer[i * channel_count + channel];
        }
    } else {
        for (std::size_t i = 0; i < static_cast<std::size_t>(samples_processed); i++) {
            sample_buffer[mix_offset + i] =
                static_cast<s32>(static_cast<f32>(buffer[i * channel_count + channel] /
                                                  std::numeric_limits<s32>::max()) *
                                 std::numeric_limits<s16>::max());
        }
    }

    return samples_processed;
}

s32 CommandGenerator::DecodeAdpcm(ServerVoiceInfo& voice_info, VoiceState& dsp_state,
                                  s32 sample_start_offset, s32 sample_end_offset, s32 sample_count,
                                  [[maybe_unused]] s32 channel, std::size_t mix_offset) {
    const auto& in_params = voice_info.GetInParams();
    const auto& wave_buffer = in_params.wave_buffer[dsp_state.wave_buffer_index];
    if (wave_buffer.buffer_address == 0) {
        return 0;
    }
    if (wave_buffer.buffer_size == 0) {
        return 0;
    }
    if (sample_end_offset < sample_start_offset) {
        return 0;
    }

    static constexpr std::array<int, 16> SIGNED_NIBBLES{
        0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1,
    };

    constexpr std::size_t FRAME_LEN = 8;
    constexpr std::size_t NIBBLES_PER_SAMPLE = 16;
    constexpr std::size_t SAMPLES_PER_FRAME = 14;

    auto frame_header = dsp_state.context.header;
    s32 idx = (frame_header >> 4) & 0xf;
    s32 scale = frame_header & 0xf;
    s16 yn1 = dsp_state.context.yn1;
    s16 yn2 = dsp_state.context.yn2;

    Codec::ADPCM_Coeff coeffs;
    memory.ReadBlock(in_params.additional_params_address, coeffs.data(),
                     sizeof(Codec::ADPCM_Coeff));

    s32 coef1 = coeffs[idx * 2];
    s32 coef2 = coeffs[idx * 2 + 1];

    const auto samples_remaining = (sample_end_offset - sample_start_offset) - dsp_state.offset;
    const auto samples_processed = std::min(sample_count, samples_remaining);
    const auto sample_pos = dsp_state.offset + sample_start_offset;

    const auto samples_remaining_in_frame = sample_pos % SAMPLES_PER_FRAME;
    auto position_in_frame = ((sample_pos / SAMPLES_PER_FRAME) * NIBBLES_PER_SAMPLE) +
                             samples_remaining_in_frame + (samples_remaining_in_frame != 0 ? 2 : 0);

    const auto decode_sample = [&](const int nibble) -> s16 {
        const int xn = nibble * (1 << scale);
        // We first transform everything into 11 bit fixed point, perform the second order
        // digital filter, then transform back.
        // 0x400 == 0.5 in 11 bit fixed point.
        // Filter: y[n] = x[n] + 0.5 + c1 * y[n-1] + c2 * y[n-2]
        int val = ((xn << 11) + 0x400 + coef1 * yn1 + coef2 * yn2) >> 11;
        // Clamp to output range.
        val = std::clamp<s32>(val, -32768, 32767);
        // Advance output feedback.
        yn2 = yn1;
        yn1 = static_cast<s16>(val);
        return yn1;
    };

    std::size_t buffer_offset{};
    std::vector<u8> buffer(
        std::max((samples_processed / FRAME_LEN) * SAMPLES_PER_FRAME, FRAME_LEN));
    memory.ReadBlock(wave_buffer.buffer_address + (position_in_frame / 2), buffer.data(),
                     buffer.size());
    std::size_t cur_mix_offset = mix_offset;

    auto remaining_samples = samples_processed;
    while (remaining_samples > 0) {
        if (position_in_frame % NIBBLES_PER_SAMPLE == 0) {
            // Read header
            frame_header = buffer[buffer_offset++];
            idx = (frame_header >> 4) & 0xf;
            scale = frame_header & 0xf;
            coef1 = coeffs[idx * 2];
            coef2 = coeffs[idx * 2 + 1];
            position_in_frame += 2;

            // Decode entire frame
            if (remaining_samples >= static_cast<int>(SAMPLES_PER_FRAME)) {
                for (std::size_t i = 0; i < SAMPLES_PER_FRAME / 2; i++) {
                    // Sample 1
                    const s32 s0 = SIGNED_NIBBLES[buffer[buffer_offset] >> 4];
                    const s32 s1 = SIGNED_NIBBLES[buffer[buffer_offset++] & 0xf];
                    const s16 sample_1 = decode_sample(s0);
                    const s16 sample_2 = decode_sample(s1);
                    sample_buffer[cur_mix_offset++] = sample_1;
                    sample_buffer[cur_mix_offset++] = sample_2;
                }
                remaining_samples -= static_cast<int>(SAMPLES_PER_FRAME);
                position_in_frame += SAMPLES_PER_FRAME;
                continue;
            }
        }
        // Decode mid frame
        s32 current_nibble = buffer[buffer_offset];
        if (position_in_frame++ & 0x1) {
            current_nibble &= 0xf;
            buffer_offset++;
        } else {
            current_nibble >>= 4;
        }
        const s16 sample = decode_sample(SIGNED_NIBBLES[current_nibble]);
        sample_buffer[cur_mix_offset++] = sample;
        remaining_samples--;
    }

    dsp_state.context.header = frame_header;
    dsp_state.context.yn1 = yn1;
    dsp_state.context.yn2 = yn2;

    return samples_processed;
}

std::span<s32> CommandGenerator::GetMixBuffer(std::size_t index) {
    return std::span<s32>(mix_buffer.data() + (index * worker_params.sample_count),
                          worker_params.sample_count);
}

std::span<const s32> CommandGenerator::GetMixBuffer(std::size_t index) const {
    return std::span<const s32>(mix_buffer.data() + (index * worker_params.sample_count),
                                worker_params.sample_count);
}

std::size_t CommandGenerator::GetMixChannelBufferOffset(s32 channel) const {
    return worker_params.mix_buffer_count + channel;
}

std::size_t CommandGenerator::GetTotalMixBufferCount() const {
    return worker_params.mix_buffer_count + AudioCommon::MAX_CHANNEL_COUNT;
}

std::span<s32> CommandGenerator::GetChannelMixBuffer(s32 channel) {
    return GetMixBuffer(worker_params.mix_buffer_count + channel);
}

std::span<const s32> CommandGenerator::GetChannelMixBuffer(s32 channel) const {
    return GetMixBuffer(worker_params.mix_buffer_count + channel);
}

void CommandGenerator::DecodeFromWaveBuffers(ServerVoiceInfo& voice_info, std::span<s32> output,
                                             VoiceState& dsp_state, s32 channel,
                                             s32 target_sample_rate, s32 sample_count,
                                             s32 node_id) {
    const auto& in_params = voice_info.GetInParams();
    if (dumping_frame) {
        LOG_DEBUG(Audio,
                  "(DSP_TRACE) DecodeFromWaveBuffers, node_id={}, channel={}, "
                  "format={}, sample_count={}, sample_rate={}, mix_id={}, splitter_id={}",
                  node_id, channel, in_params.sample_format, sample_count, in_params.sample_rate,
                  in_params.mix_id, in_params.splitter_info_id);
    }
    ASSERT_OR_EXECUTE(output.data() != nullptr, { return; });

    const auto resample_rate = static_cast<s32>(
        static_cast<float>(in_params.sample_rate) / static_cast<float>(target_sample_rate) *
        static_cast<float>(static_cast<s32>(in_params.pitch * 32768.0f)));
    if (dsp_state.fraction + sample_count * resample_rate >
        static_cast<s32>(SCALED_MIX_BUFFER_SIZE - 4ULL)) {
        return;
    }

    auto min_required_samples =
        std::min(static_cast<s32>(SCALED_MIX_BUFFER_SIZE) - dsp_state.fraction, resample_rate);
    if (min_required_samples >= sample_count) {
        min_required_samples = sample_count;
    }

    std::size_t temp_mix_offset{};
    s32 samples_output{};
    auto samples_remaining = sample_count;
    while (samples_remaining > 0) {
        const auto samples_to_output = std::min(samples_remaining, min_required_samples);
        const auto samples_to_read = (samples_to_output * resample_rate + dsp_state.fraction) >> 15;

        if (!in_params.behavior_flags.is_pitch_and_src_skipped) {
            // Append sample histtory for resampler
            for (std::size_t i = 0; i < AudioCommon::MAX_SAMPLE_HISTORY; i++) {
                sample_buffer[temp_mix_offset + i] = dsp_state.sample_history[i];
            }
            temp_mix_offset += 4;
        }

        s32 samples_read{};
        while (samples_read < samples_to_read) {
            const auto& wave_buffer = in_params.wave_buffer[dsp_state.wave_buffer_index];
            // No more data can be read
            if (!dsp_state.is_wave_buffer_valid[dsp_state.wave_buffer_index]) {
                break;
            }

            if (in_params.sample_format == SampleFormat::Adpcm && dsp_state.offset == 0 &&
                wave_buffer.context_address != 0 && wave_buffer.context_size != 0) {
                memory.ReadBlock(wave_buffer.context_address, &dsp_state.context,
                                 sizeof(ADPCMContext));
            }

            s32 samples_offset_start;
            s32 samples_offset_end;
            if (dsp_state.loop_count > 0 && wave_buffer.loop_start_sample != 0 &&
                wave_buffer.loop_end_sample != 0 &&
                wave_buffer.loop_start_sample <= wave_buffer.loop_end_sample) {
                samples_offset_start = wave_buffer.loop_start_sample;
                samples_offset_end = wave_buffer.loop_end_sample;
            } else {
                samples_offset_start = wave_buffer.start_sample_offset;
                samples_offset_end = wave_buffer.end_sample_offset;
            }

            s32 samples_decoded{0};
            switch (in_params.sample_format) {
            case SampleFormat::Pcm8:
                samples_decoded =
                    DecodePcm<s8>(voice_info, dsp_state, samples_offset_start, samples_offset_end,
                                  samples_to_read - samples_read, channel, temp_mix_offset);
                break;
            case SampleFormat::Pcm16:
                samples_decoded =
                    DecodePcm<s16>(voice_info, dsp_state, samples_offset_start, samples_offset_end,
                                   samples_to_read - samples_read, channel, temp_mix_offset);
                break;
            case SampleFormat::Pcm32:
                samples_decoded =
                    DecodePcm<s32>(voice_info, dsp_state, samples_offset_start, samples_offset_end,
                                   samples_to_read - samples_read, channel, temp_mix_offset);
                break;
            case SampleFormat::PcmFloat:
                samples_decoded =
                    DecodePcm<f32>(voice_info, dsp_state, samples_offset_start, samples_offset_end,
                                   samples_to_read - samples_read, channel, temp_mix_offset);
                break;
            case SampleFormat::Adpcm:
                samples_decoded =
                    DecodeAdpcm(voice_info, dsp_state, samples_offset_start, samples_offset_end,
                                samples_to_read - samples_read, channel, temp_mix_offset);
                break;
            default:
                ASSERT_MSG(false, "Unimplemented sample format={}", in_params.sample_format);
            }

            temp_mix_offset += samples_decoded;
            samples_read += samples_decoded;
            dsp_state.offset += samples_decoded;
            dsp_state.played_sample_count += samples_decoded;

            if (dsp_state.offset >= (samples_offset_end - samples_offset_start) ||
                samples_decoded == 0) {
                // Reset our sample offset
                dsp_state.offset = 0;
                if (wave_buffer.is_looping) {
                    dsp_state.loop_count++;
                    if (wave_buffer.loop_count > 0 &&
                        (dsp_state.loop_count > wave_buffer.loop_count || samples_decoded == 0)) {
                        // End of our buffer
                        voice_info.SetWaveBufferCompleted(dsp_state, wave_buffer);
                    }

                    if (samples_decoded == 0) {
                        break;
                    }

                    if (in_params.behavior_flags.is_played_samples_reset_at_loop_point.Value()) {
                        dsp_state.played_sample_count = 0;
                    }
                } else {
                    // Update our wave buffer states
                    voice_info.SetWaveBufferCompleted(dsp_state, wave_buffer);
                }
            }
        }

        if (in_params.behavior_flags.is_pitch_and_src_skipped.Value()) {
            // No need to resample
            std::memcpy(output.data() + samples_output, sample_buffer.data(),
                        samples_read * sizeof(s32));
        } else {
            std::fill(sample_buffer.begin() + temp_mix_offset,
                      sample_buffer.begin() + temp_mix_offset + (samples_to_read - samples_read),
                      0);
            AudioCore::Resample(output.data() + samples_output, sample_buffer.data(), resample_rate,
                                dsp_state.fraction, samples_to_output);
            // Resample
            for (std::size_t i = 0; i < AudioCommon::MAX_SAMPLE_HISTORY; i++) {
                dsp_state.sample_history[i] = sample_buffer[samples_to_read + i];
            }
        }
        samples_remaining -= samples_to_output;
        samples_output += samples_to_output;
    }
}

} // namespace AudioCore
