// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/delay.h"

namespace AudioCore::AudioRenderer {
/**
 * Update the DelayInfo state according to the given parameters.
 *
 * @param params - Input parameters to update the state.
 * @param state  - State to be updated.
 */
static void SetDelayEffectParameter(const DelayInfo::ParameterVersion1& params,
                                    DelayInfo::State& state) {
    auto channel_spread{params.channel_spread};
    state.feedback_gain = params.feedback_gain * 0.97998046875f;
    state.delay_feedback_gain = state.feedback_gain * (1.0f - channel_spread);
    if (params.channel_count == 4 || params.channel_count == 6) {
        channel_spread >>= 1;
    }
    state.delay_feedback_cross_gain = channel_spread * state.feedback_gain;
    state.lowpass_feedback_gain = params.lowpass_amount * 0.949951171875f;
    state.lowpass_gain = 1.0f - state.lowpass_feedback_gain;
}

/**
 * Initialize a new DelayInfo state according to the given parameters.
 *
 * @param params     - Input parameters to update the state.
 * @param state      - State to be updated.
 * @param workbuffer - Game-supplied memory for the state. (Unused)
 */
static void InitializeDelayEffect(const DelayInfo::ParameterVersion1& params,
                                  DelayInfo::State& state,
                                  [[maybe_unused]] const CpuAddr workbuffer) {
    state = {};

    for (u32 channel = 0; channel < params.channel_count; channel++) {
        Common::FixedPoint<32, 32> sample_count_max{0.064f};
        sample_count_max *= params.sample_rate.to_int_floor() * params.delay_time_max;

        Common::FixedPoint<18, 14> delay_time{params.delay_time};
        delay_time *= params.sample_rate / 1000;
        Common::FixedPoint<32, 32> sample_count{delay_time};

        if (sample_count > sample_count_max) {
            sample_count = sample_count_max;
        }

        state.delay_lines[channel].sample_count_max = sample_count_max.to_int_floor();
        state.delay_lines[channel].sample_count = sample_count.to_int_floor();
        state.delay_lines[channel].buffer.resize(state.delay_lines[channel].sample_count, 0);
        if (state.delay_lines[channel].buffer.size() == 0) {
            state.delay_lines[channel].buffer.push_back(0);
        }
        state.delay_lines[channel].buffer_pos = 0;
        state.delay_lines[channel].decay_rate = 1.0f;
    }

    SetDelayEffectParameter(params, state);
}

/**
 * Delay effect impl, according to the parameters and current state, on the input mix buffers,
 * saving the results to the output mix buffers.
 *
 * @param params       - Input parameters to use.
 * @param state        - State to use, must be initialized (see InitializeDelayEffect).
 * @param inputs       - Input mix buffers to performan the delay on.
 * @param outputs      - Output mix buffers to receive the delayed samples.
 * @param sample_count - Number of samples to process.
 */
template <size_t Channels>
static void ApplyDelay(const DelayInfo::ParameterVersion1& params, DelayInfo::State& state,
                       std::vector<std::span<const s32>>& inputs,
                       std::vector<std::span<s32>>& outputs, const u32 sample_count) {
    for (u32 i = 0; i < sample_count; i++) {
        std::array<Common::FixedPoint<50, 14>, Channels> input_samples{};
        for (u32 channel = 0; channel < Channels; channel++) {
            input_samples[channel] = inputs[channel][i] * 64;
        }

        std::array<Common::FixedPoint<50, 14>, Channels> delay_samples{};
        for (u32 channel = 0; channel < Channels; channel++) {
            delay_samples[channel] = state.delay_lines[channel].Read();
        }

        std::array<std::array<Common::FixedPoint<18, 14>, Channels>, Channels> matrix{};
        if constexpr (Channels == 1) {
            matrix = {{
                {state.feedback_gain},
            }};
        } else if constexpr (Channels == 2) {
            matrix = {{
                {state.delay_feedback_gain, state.delay_feedback_cross_gain},
                {state.delay_feedback_cross_gain, state.delay_feedback_gain},
            }};
        } else if constexpr (Channels == 4) {
            matrix = {{
                {state.delay_feedback_gain, state.delay_feedback_cross_gain,
                 state.delay_feedback_cross_gain, 0.0f},
                {state.delay_feedback_cross_gain, state.delay_feedback_gain, 0.0f,
                 state.delay_feedback_cross_gain},
                {state.delay_feedback_cross_gain, 0.0f, state.delay_feedback_gain,
                 state.delay_feedback_cross_gain},
                {0.0f, state.delay_feedback_cross_gain, state.delay_feedback_cross_gain,
                 state.delay_feedback_gain},
            }};
        } else if constexpr (Channels == 6) {
            matrix = {{
                {state.delay_feedback_gain, 0.0f, 0.0f, 0.0f, state.delay_feedback_cross_gain,
                 state.delay_feedback_cross_gain},
                {0.0f, state.delay_feedback_gain, 0.0f, state.delay_feedback_cross_gain,
                 state.delay_feedback_cross_gain, 0.0f},
                {state.delay_feedback_cross_gain, 0.0f, state.delay_feedback_gain,
                 state.delay_feedback_cross_gain, 0.0f, 0.0f},
                {0.0f, state.delay_feedback_cross_gain, state.delay_feedback_cross_gain,
                 state.delay_feedback_gain, 0.0f, 0.0f},
                {state.delay_feedback_cross_gain, state.delay_feedback_cross_gain, 0.0f, 0.0f,
                 state.delay_feedback_gain, 0.0f},
                {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, params.feedback_gain},
            }};
        }

        std::array<Common::FixedPoint<50, 14>, Channels> gained_samples{};
        for (u32 channel = 0; channel < Channels; channel++) {
            Common::FixedPoint<50, 14> delay{};
            for (u32 j = 0; j < Channels; j++) {
                delay += delay_samples[j] * matrix[j][channel];
            }
            gained_samples[channel] = input_samples[channel] * params.in_gain + delay;
        }

        for (u32 channel = 0; channel < Channels; channel++) {
            state.lowpass_z[channel] = gained_samples[channel] * state.lowpass_gain +
                                       state.lowpass_z[channel] * state.lowpass_feedback_gain;
            state.delay_lines[channel].Write(state.lowpass_z[channel]);
        }

        for (u32 channel = 0; channel < Channels; channel++) {
            outputs[channel][i] = (input_samples[channel] * params.dry_gain +
                                   delay_samples[channel] * params.wet_gain)
                                      .to_int_floor() /
                                  64;
        }
    }
}

/**
 * Apply a delay effect if enabled, according to the parameters and current state, on the input mix
 * buffers, saving the results to the output mix buffers.
 *
 * @param params       - Input parameters to use.
 * @param state        - State to use, must be initialized (see InitializeDelayEffect).
 * @param enabled      - If enabled, delay will be applied, otherwise input is copied to output.
 * @param inputs       - Input mix buffers to performan the delay on.
 * @param outputs      - Output mix buffers to receive the delayed samples.
 * @param sample_count - Number of samples to process.
 */
static void ApplyDelayEffect(const DelayInfo::ParameterVersion1& params, DelayInfo::State& state,
                             const bool enabled, std::vector<std::span<const s32>>& inputs,
                             std::vector<std::span<s32>>& outputs, const u32 sample_count) {

    if (!IsChannelCountValid(params.channel_count)) {
        LOG_ERROR(Service_Audio, "Invalid delay channels {}", params.channel_count);
        return;
    }

    if (enabled) {
        switch (params.channel_count) {
        case 1:
            ApplyDelay<1>(params, state, inputs, outputs, sample_count);
            break;
        case 2:
            ApplyDelay<2>(params, state, inputs, outputs, sample_count);
            break;
        case 4:
            ApplyDelay<4>(params, state, inputs, outputs, sample_count);
            break;
        case 6:
            ApplyDelay<6>(params, state, inputs, outputs, sample_count);
            break;
        default:
            for (u32 channel = 0; channel < params.channel_count; channel++) {
                if (inputs[channel].data() != outputs[channel].data()) {
                    std::memcpy(outputs[channel].data(), inputs[channel].data(),
                                sample_count * sizeof(s32));
                }
            }
            break;
        }
    } else {
        for (u32 channel = 0; channel < params.channel_count; channel++) {
            if (inputs[channel].data() != outputs[channel].data()) {
                std::memcpy(outputs[channel].data(), inputs[channel].data(),
                            sample_count * sizeof(s32));
            }
        }
    }
}

void DelayCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                        std::string& string) {
    string += fmt::format("DelayCommand\n\tenabled {} \n\tinputs: ", effect_enabled);
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n\toutputs: ";
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", outputs[i]);
    }
    string += "\n";
}

void DelayCommand::Process(const ADSP::CommandListProcessor& processor) {
    std::vector<std::span<const s32>> input_buffers(parameter.channel_count);
    std::vector<std::span<s32>> output_buffers(parameter.channel_count);

    for (s16 i = 0; i < parameter.channel_count; i++) {
        input_buffers[i] = processor.mix_buffers.subspan(inputs[i] * processor.sample_count,
                                                         processor.sample_count);
        output_buffers[i] = processor.mix_buffers.subspan(outputs[i] * processor.sample_count,
                                                          processor.sample_count);
    }

    auto state_{reinterpret_cast<DelayInfo::State*>(state)};

    if (effect_enabled) {
        if (parameter.state == DelayInfo::ParameterState::Updating) {
            SetDelayEffectParameter(parameter, *state_);
        } else if (parameter.state == DelayInfo::ParameterState::Initialized) {
            InitializeDelayEffect(parameter, *state_, workbuffer);
        }
    }
    ApplyDelayEffect(parameter, *state_, effect_enabled, input_buffers, output_buffers,
                     processor.sample_count);
}

bool DelayCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
