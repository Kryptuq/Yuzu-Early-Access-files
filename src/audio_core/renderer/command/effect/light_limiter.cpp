// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/light_limiter.h"

namespace AudioCore::AudioRenderer {
/**
 * Update the LightLimiterInfo state according to the given parameters.
 * A no-op.
 *
 * @param params - Input parameters to update the state.
 * @param state  - State to be updated.
 */
static void UpdateLightLimiterEffectParameter(const LightLimiterInfo::ParameterVersion2& params,
                                              LightLimiterInfo::State& state) {}

/**
 * Initialize a new LightLimiterInfo state according to the given parameters.
 *
 * @param params     - Input parameters to update the state.
 * @param state      - State to be updated.
 * @param workbuffer - Game-supplied memory for the state. (Unused)
 */
static void InitializeLightLimiterEffect(const LightLimiterInfo::ParameterVersion2& params,
                                         LightLimiterInfo::State& state, const CpuAddr workbuffer) {
    state = {};
    state.samples_average.fill(0.0f);
    state.compression_gain.fill(1.0f);
    state.look_ahead_sample_offsets.fill(0);
    for (u32 i = 0; i < params.channel_count; i++) {
        state.look_ahead_sample_buffers[i].resize(params.look_ahead_samples_max, 0.0f);
    }
}

/**
 * Apply a light limiter effect if enabled, according to the current state, on the input mix
 * buffers, saving the results to the output mix buffers.
 *
 * @param params       - Input parameters to use.
 * @param state        - State to use, must be initialized (see InitializeLightLimiterEffect).
 * @param enabled      - If enabled, limiter will be applied, otherwise input is copied to output.
 * @param inputs       - Input mix buffers to perform the limiter on.
 * @param outputs      - Output mix buffers to receive the limited samples.
 * @param sample_count - Number of samples to process.
 * @params statistics  - Optional output statistics, only used with version 2.
 */
static void ApplyLightLimiterEffect(const LightLimiterInfo::ParameterVersion2& params,
                                    LightLimiterInfo::State& state, const bool enabled,
                                    std::vector<std::span<const s32>>& inputs,
                                    std::vector<std::span<s32>>& outputs, const u32 sample_count,
                                    LightLimiterInfo::StatisticsInternal* statistics) {
    constexpr s64 min{std::numeric_limits<s32>::min()};
    constexpr s64 max{std::numeric_limits<s32>::max()};

    const auto recip_estimate = [](f64 a) -> f64 {
        s32 q, s;
        f64 r;
        q = (s32)(a * 512.0);               /* a in units of 1/512 rounded down */
        r = 1.0 / (((f64)q + 0.5) / 512.0); /* reciprocal r */
        s = (s32)(256.0 * r + 0.5);         /* r in units of 1/256 rounded to nearest */
        return ((f64)s / 256.0);
    };

    if (enabled) {
        if (statistics && params.statistics_reset_required) {
            for (u32 i = 0; i < params.channel_count; i++) {
                statistics->channel_compression_gain_min[i] = 1.0f;
                statistics->channel_max_sample[i] = 0;
            }
        }

        for (u32 sample_index = 0; sample_index < sample_count; sample_index++) {
            for (u32 channel = 0; channel < params.channel_count; channel++) {
                auto sample{(Common::FixedPoint<49, 15>(inputs[channel][sample_index]) /
                             Common::FixedPoint<49, 15>::one) *
                            params.input_gain};
                auto abs_sample{sample};
                if (sample < 0.0f) {
                    abs_sample = -sample;
                }
                auto coeff{abs_sample > state.samples_average[channel] ? params.attack_coeff
                                                                       : params.release_coeff};
                state.samples_average[channel] +=
                    ((abs_sample - state.samples_average[channel]) * coeff).to_float();

                // Reciprocal estimate
                auto new_average_sample{Common::FixedPoint<49, 15>(
                    recip_estimate(state.samples_average[channel].to_double()))};
                if (params.processing_mode != LightLimiterInfo::ProcessingMode::Mode1) {
                    // Two Newton-Raphson steps
                    auto temp{2.0 - (state.samples_average[channel] * new_average_sample)};
                    new_average_sample = 2.0 - (state.samples_average[channel] * temp);
                }

                auto above_threshold{state.samples_average[channel] > params.threshold};
                auto attenuation{above_threshold ? params.threshold * new_average_sample : 1.0f};
                coeff = attenuation < state.compression_gain[channel] ? params.attack_coeff
                                                                      : params.release_coeff;
                state.compression_gain[channel] +=
                    (attenuation - state.compression_gain[channel]) * coeff;

                auto lookahead_sample{
                    state.look_ahead_sample_buffers[channel]
                                                   [state.look_ahead_sample_offsets[channel]]};

                state.look_ahead_sample_buffers[channel][state.look_ahead_sample_offsets[channel]] =
                    sample;
                state.look_ahead_sample_offsets[channel] =
                    (state.look_ahead_sample_offsets[channel] + 1) % params.look_ahead_samples_min;

                outputs[channel][sample_index] = static_cast<s32>(
                    std::clamp((lookahead_sample * state.compression_gain[channel] *
                                params.output_gain * Common::FixedPoint<49, 15>::one)
                                   .to_long(),
                               min, max));

                if (statistics) {
                    statistics->channel_max_sample[channel] =
                        std::max(statistics->channel_max_sample[channel], abs_sample.to_float());
                    statistics->channel_compression_gain_min[channel] =
                        std::min(statistics->channel_compression_gain_min[channel],
                                 state.compression_gain[channel].to_float());
                }
            }
        }
    } else {
        for (u32 i = 0; i < params.channel_count; i++) {
            if (params.inputs[i] != params.outputs[i]) {
                std::memcpy(outputs[i].data(), inputs[i].data(), outputs[i].size_bytes());
            }
        }
    }
}

void LightLimiterVersion1Command::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                                       std::string& string) {
    string += fmt::format("LightLimiterVersion1Command\n\tinputs: ");
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n\toutputs: ";
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", outputs[i]);
    }
    string += "\n";
}

void LightLimiterVersion1Command::Process(const ADSP::CommandListProcessor& processor) {
    std::vector<std::span<const s32>> input_buffers(parameter.channel_count);
    std::vector<std::span<s32>> output_buffers(parameter.channel_count);

    for (u32 i = 0; i < parameter.channel_count; i++) {
        input_buffers[i] = processor.mix_buffers.subspan(inputs[i] * processor.sample_count,
                                                         processor.sample_count);
        output_buffers[i] = processor.mix_buffers.subspan(outputs[i] * processor.sample_count,
                                                          processor.sample_count);
    }

    auto state_{reinterpret_cast<LightLimiterInfo::State*>(state)};

    if (effect_enabled) {
        if (parameter.state == LightLimiterInfo::ParameterState::Updating) {
            UpdateLightLimiterEffectParameter(parameter, *state_);
        } else if (parameter.state == LightLimiterInfo::ParameterState::Initialized) {
            InitializeLightLimiterEffect(parameter, *state_, workbuffer);
        }
    }

    LightLimiterInfo::StatisticsInternal* statistics{nullptr};
    ApplyLightLimiterEffect(parameter, *state_, effect_enabled, input_buffers, output_buffers,
                            processor.sample_count, statistics);
}

bool LightLimiterVersion1Command::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

void LightLimiterVersion2Command::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                                       std::string& string) {
    string += fmt::format("LightLimiterVersion2Command\n\tinputs: \n");
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n\toutputs: ";
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", outputs[i]);
    }
    string += "\n";
}

void LightLimiterVersion2Command::Process(const ADSP::CommandListProcessor& processor) {
    std::vector<std::span<const s32>> input_buffers(parameter.channel_count);
    std::vector<std::span<s32>> output_buffers(parameter.channel_count);

    for (u32 i = 0; i < parameter.channel_count; i++) {
        input_buffers[i] = processor.mix_buffers.subspan(inputs[i] * processor.sample_count,
                                                         processor.sample_count);
        output_buffers[i] = processor.mix_buffers.subspan(outputs[i] * processor.sample_count,
                                                          processor.sample_count);
    }

    auto state_{reinterpret_cast<LightLimiterInfo::State*>(state)};

    if (effect_enabled) {
        if (parameter.state == LightLimiterInfo::ParameterState::Updating) {
            UpdateLightLimiterEffectParameter(parameter, *state_);
        } else if (parameter.state == LightLimiterInfo::ParameterState::Initialized) {
            InitializeLightLimiterEffect(parameter, *state_, workbuffer);
        }
    }

    auto statistics{reinterpret_cast<LightLimiterInfo::StatisticsInternal*>(result_state)};
    ApplyLightLimiterEffect(parameter, *state_, effect_enabled, input_buffers, output_buffers,
                            processor.sample_count, statistics);
}

bool LightLimiterVersion2Command::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
