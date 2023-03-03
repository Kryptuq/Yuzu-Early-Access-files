// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <numbers>
#include <ranges>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/reverb.h"
#include "common/polyfill_ranges.h"

namespace AudioCore::AudioRenderer {

constexpr std::array<f32, ReverbInfo::MaxDelayLines> FdnMaxDelayLineTimes = {
    53.9532470703125f,
    79.19256591796875f,
    116.23876953125f,
    170.61529541015625f,
};

constexpr std::array<f32, ReverbInfo::MaxDelayLines> DecayMaxDelayLineTimes = {
    7.0f,
    9.0f,
    13.0f,
    17.0f,
};

constexpr std::array<std::array<f32, ReverbInfo::MaxDelayTaps + 1>, ReverbInfo::NumEarlyModes>
    EarlyDelayTimes = {
        {{0.000000f, 3.500000f, 2.799988f, 3.899963f, 2.699951f, 13.399963f, 7.899963f, 8.399963f,
          9.899963f, 12.000000f, 12.500000f},
         {0.000000f, 11.799988f, 5.500000f, 11.199951f, 10.399963f, 38.099976f, 22.199951f,
          29.599976f, 21.199951f, 24.799988f, 40.000000f},
         {0.000000f, 41.500000f, 20.500000f, 41.299988f, 0.000000f, 29.500000f, 33.799988f,
          45.199951f, 46.799988f, 0.000000f, 50.000000f},
         {33.099976f, 43.299988f, 22.799988f, 37.899963f, 14.899963f, 35.299988f, 17.899963f,
          34.199951f, 0.000000f, 43.299988f, 50.000000f},
         {0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
          0.000000f, 0.000000f, 0.000000f}},
};

constexpr std::array<std::array<f32, ReverbInfo::MaxDelayTaps>, ReverbInfo::NumEarlyModes>
    EarlyDelayGains = {{
        {0.699951f, 0.679993f, 0.699951f, 0.679993f, 0.699951f, 0.679993f, 0.699951f, 0.679993f,
         0.679993f, 0.679993f},
        {0.699951f, 0.679993f, 0.699951f, 0.679993f, 0.699951f, 0.679993f, 0.679993f, 0.679993f,
         0.679993f, 0.679993f},
        {0.500000f, 0.699951f, 0.699951f, 0.679993f, 0.500000f, 0.679993f, 0.679993f, 0.699951f,
         0.679993f, 0.000000f},
        {0.929993f, 0.919983f, 0.869995f, 0.859985f, 0.939941f, 0.809998f, 0.799988f, 0.769958f,
         0.759949f, 0.649963f},
        {0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
         0.000000f, 0.000000f},
    }};

constexpr std::array<std::array<f32, ReverbInfo::MaxDelayLines>, ReverbInfo::NumLateModes>
    FdnDelayTimes = {{
        {53.953247f, 79.192566f, 116.238770f, 130.615295f},
        {53.953247f, 79.192566f, 116.238770f, 170.615295f},
        {5.000000f, 10.000000f, 5.000000f, 10.000000f},
        {47.029968f, 71.000000f, 103.000000f, 170.000000f},
        {53.953247f, 79.192566f, 116.238770f, 170.615295f},
    }};

constexpr std::array<std::array<f32, ReverbInfo::MaxDelayLines>, ReverbInfo::NumLateModes>
    DecayDelayTimes = {{
        {7.000000f, 9.000000f, 13.000000f, 17.000000f},
        {7.000000f, 9.000000f, 13.000000f, 17.000000f},
        {1.000000f, 1.000000f, 1.000000f, 1.000000f},
        {7.000000f, 7.000000f, 13.000000f, 9.000000f},
        {7.000000f, 9.000000f, 13.000000f, 17.000000f},
    }};

/**
 * Update the ReverbInfo state according to the given parameters.
 *
 * @param params - Input parameters to update the state.
 * @param state  - State to be updated.
 */
static void UpdateReverbEffectParameter(const ReverbInfo::ParameterVersion2& params,
                                        ReverbInfo::State& state) {
    const auto pow_10 = [](f32 val) -> f32 {
        return (val >= 0.0f) ? 1.0f : (val <= -5.3f) ? 0.0f : std::pow(10.0f, val);
    };
    const auto cos = [](f32 degrees) -> f32 {
        return std::cos(degrees * std::numbers::pi_v<f32> / 180.0f);
    };

    static bool unk_initialized{false};
    static Common::FixedPoint<50, 14> unk_value{};

    const auto sample_rate{Common::FixedPoint<50, 14>::from_base(params.sample_rate)};
    const auto pre_delay_time{Common::FixedPoint<50, 14>::from_base(params.pre_delay)};

    for (u32 i = 0; i < ReverbInfo::MaxDelayTaps; i++) {
        auto early_delay{
            ((pre_delay_time + EarlyDelayTimes[params.early_mode][i]) * sample_rate).to_int()};
        early_delay = std::min(early_delay, state.pre_delay_line.sample_count_max);
        state.early_delay_times[i] = early_delay + 1;
        state.early_gains[i] = Common::FixedPoint<50, 14>::from_base(params.early_gain) *
                               EarlyDelayGains[params.early_mode][i];
    }

    if (params.channel_count == 2) {
        state.early_gains[4] * 0.5f;
        state.early_gains[5] * 0.5f;
    }

    auto pre_time{
        ((pre_delay_time + EarlyDelayTimes[params.early_mode][10]) * sample_rate).to_int()};
    state.pre_delay_time = std::min(pre_time, state.pre_delay_line.sample_count_max);

    if (!unk_initialized) {
        unk_value = cos((1280.0f / sample_rate).to_float());
        unk_initialized = true;
    }

    for (u32 i = 0; i < ReverbInfo::MaxDelayLines; i++) {
        const auto fdn_delay{(FdnDelayTimes[params.late_mode][i] * sample_rate).to_int()};
        state.fdn_delay_lines[i].sample_count =
            std::min(fdn_delay, state.fdn_delay_lines[i].sample_count_max);
        state.fdn_delay_lines[i].buffer_end =
            &state.fdn_delay_lines[i].buffer[state.fdn_delay_lines[i].sample_count - 1];

        const auto decay_delay{(DecayDelayTimes[params.late_mode][i] * sample_rate).to_int()};
        state.decay_delay_lines[i].sample_count =
            std::min(decay_delay, state.decay_delay_lines[i].sample_count_max);
        state.decay_delay_lines[i].buffer_end =
            &state.decay_delay_lines[i].buffer[state.decay_delay_lines[i].sample_count - 1];

        state.decay_delay_lines[i].decay =
            0.5999755859375f * (1.0f - Common::FixedPoint<50, 14>::from_base(params.colouration));

        auto a{(Common::FixedPoint<50, 14>(state.fdn_delay_lines[i].sample_count_max) +
                state.decay_delay_lines[i].sample_count_max) *
               -3};
        auto b{a / (Common::FixedPoint<50, 14>::from_base(params.decay_time) * sample_rate)};
        Common::FixedPoint<50, 14> c{0.0f};
        Common::FixedPoint<50, 14> d{0.0f};
        auto hf_decay_ratio{Common::FixedPoint<50, 14>::from_base(params.high_freq_decay_ratio)};

        if (hf_decay_ratio > 0.99493408203125f) {
            c = 0.0f;
            d = 1.0f;
        } else {
            const auto e{
                pow_10(((((1.0f / hf_decay_ratio) - 1.0f) * 2) / 100 * (b / 10)).to_float())};
            const auto f{1.0f - e};
            const auto g{2.0f - (unk_value * e * 2)};
            const auto h{std::sqrt(std::pow(g.to_float(), 2.0f) - (std::pow(f, 2.0f) * 4))};

            c = (g - h) / (f * 2.0f);
            d = 1.0f - c;
        }

        state.hf_decay_prev_gain[i] = c;
        state.hf_decay_gain[i] = pow_10((b / 1000).to_float()) * d * 0.70709228515625f;
        state.prev_feedback_output[i] = 0;
    }
}

/**
 * Initialize a new ReverbInfo state according to the given parameters.
 *
 * @param params                        - Input parameters to update the state.
 * @param state                         - State to be updated.
 * @param workbuffer                    - Game-supplied memory for the state. (Unused)
 * @param long_size_pre_delay_supported - Use a longer pre-delay time before reverb begins.
 */
static void InitializeReverbEffect(const ReverbInfo::ParameterVersion2& params,
                                   ReverbInfo::State& state, const CpuAddr workbuffer,
                                   const bool long_size_pre_delay_supported) {
    state = {};

    auto delay{Common::FixedPoint<50, 14>::from_base(params.sample_rate)};

    for (u32 i = 0; i < ReverbInfo::MaxDelayLines; i++) {
        auto fdn_delay_time{(FdnMaxDelayLineTimes[i] * delay).to_uint_floor()};
        state.fdn_delay_lines[i].Initialize(fdn_delay_time, 1.0f);

        auto decay_delay_time{(DecayMaxDelayLineTimes[i] * delay).to_uint_floor()};
        state.decay_delay_lines[i].Initialize(decay_delay_time, 0.0f);
    }

    const auto pre_delay{long_size_pre_delay_supported ? 350.0f : 150.0f};
    const auto pre_delay_line{(pre_delay * delay).to_uint_floor()};
    state.pre_delay_line.Initialize(pre_delay_line, 1.0f);

    const auto center_delay_time{(5 * delay).to_uint_floor()};
    state.center_delay_line.Initialize(center_delay_time, 1.0f);

    UpdateReverbEffectParameter(params, state);

    for (u32 i = 0; i < ReverbInfo::MaxDelayLines; i++) {
        std::ranges::fill(state.fdn_delay_lines[i].buffer, 0);
        std::ranges::fill(state.decay_delay_lines[i].buffer, 0);
    }
    std::ranges::fill(state.center_delay_line.buffer, 0);
    std::ranges::fill(state.pre_delay_line.buffer, 0);
}

/**
 * Pass-through the effect, copying input to output directly, with no reverb applied.
 *
 * @param inputs        - Array of input mix buffers to copy.
 * @param outputs       - Array of output mix buffers to receive copy.
 * @param channel_count - Number of channels in inputs and outputs.
 * @param sample_count  - Number of samples within each channel.
 */
static void ApplyReverbEffectBypass(std::span<std::span<const s32>> inputs,
                                    std::span<std::span<s32>> outputs, const u32 channel_count,
                                    const u32 sample_count) {
    for (u32 i = 0; i < channel_count; i++) {
        if (inputs[i].data() != outputs[i].data()) {
            std::memcpy(outputs[i].data(), inputs[i].data(), outputs[i].size_bytes());
        }
    }
}

/**
 * Tick the delay lines, reading and returning their current output, and writing a new decaying
 * sample (mix).
 *
 * @param decay  - The decay line.
 * @param fdn    - Feedback delay network.
 * @param mix    - The new calculated sample to be written and decayed.
 * @return The next delayed and decayed sample.
 */
static Common::FixedPoint<50, 14> Axfx2AllPassTick(ReverbInfo::ReverbDelayLine& decay,
                                                   ReverbInfo::ReverbDelayLine& fdn,
                                                   const Common::FixedPoint<50, 14> mix) {
    const auto val{decay.Read()};
    const auto mixed{mix - (val * decay.decay)};
    const auto out{decay.Tick(mixed) + (mixed * decay.decay)};

    fdn.Tick(out);
    return out;
}

/**
 * Impl. Apply a Reverb according to the current state, on the input mix buffers,
 * saving the results to the output mix buffers.
 *
 * @tparam NumChannels - Number of channels to process. 1-6.
                         Inputs/outputs should have this many buffers.
 * @param params       - Input parameters to update the state.
 * @param state        - State to use, must be initialized (see InitializeReverbEffect).
 * @param inputs       - Input mix buffers to perform the reverb on.
 * @param outputs      - Output mix buffers to receive the reverbed samples.
 * @param sample_count - Number of samples to process.
 */
template <size_t NumChannels>
static void ApplyReverbEffect(const ReverbInfo::ParameterVersion2& params, ReverbInfo::State& state,
                              std::vector<std::span<const s32>>& inputs,
                              std::vector<std::span<s32>>& outputs, const u32 sample_count) {
    static constexpr std::array<u8, ReverbInfo::MaxDelayTaps> OutTapIndexes1Ch{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    static constexpr std::array<u8, ReverbInfo::MaxDelayTaps> OutTapIndexes2Ch{
        0, 0, 1, 1, 0, 1, 0, 0, 1, 1,
    };
    static constexpr std::array<u8, ReverbInfo::MaxDelayTaps> OutTapIndexes4Ch{
        0, 0, 1, 1, 0, 1, 2, 2, 3, 3,
    };
    static constexpr std::array<u8, ReverbInfo::MaxDelayTaps> OutTapIndexes6Ch{
        0, 0, 1, 1, 2, 2, 4, 4, 5, 5,
    };

    std::span<const u8> tap_indexes{};
    if constexpr (NumChannels == 1) {
        tap_indexes = OutTapIndexes1Ch;
    } else if constexpr (NumChannels == 2) {
        tap_indexes = OutTapIndexes2Ch;
    } else if constexpr (NumChannels == 4) {
        tap_indexes = OutTapIndexes4Ch;
    } else if constexpr (NumChannels == 6) {
        tap_indexes = OutTapIndexes6Ch;
    }

    for (u32 sample_index = 0; sample_index < sample_count; sample_index++) {
        std::array<Common::FixedPoint<50, 14>, NumChannels> output_samples{};

        for (u32 early_tap = 0; early_tap < ReverbInfo::MaxDelayTaps; early_tap++) {
            const auto sample{state.pre_delay_line.TapOut(state.early_delay_times[early_tap]) *
                              state.early_gains[early_tap]};
            output_samples[tap_indexes[early_tap]] += sample;
            if constexpr (NumChannels == 6) {
                output_samples[static_cast<u32>(Channels::LFE)] += sample;
            }
        }

        if constexpr (NumChannels == 6) {
            output_samples[static_cast<u32>(Channels::LFE)] *= 0.2f;
        }

        Common::FixedPoint<50, 14> input_sample{};
        for (u32 channel = 0; channel < NumChannels; channel++) {
            input_sample += inputs[channel][sample_index];
        }

        input_sample *= 64;
        input_sample *= Common::FixedPoint<50, 14>::from_base(params.base_gain);
        state.pre_delay_line.Write(input_sample);

        for (u32 i = 0; i < ReverbInfo::MaxDelayLines; i++) {
            state.prev_feedback_output[i] =
                state.prev_feedback_output[i] * state.hf_decay_prev_gain[i] +
                state.fdn_delay_lines[i].Read() * state.hf_decay_gain[i];
        }

        Common::FixedPoint<50, 14> pre_delay_sample{
            state.pre_delay_line.TapOut(state.pre_delay_time) *
            Common::FixedPoint<50, 14>::from_base(params.late_gain)};

        std::array<Common::FixedPoint<50, 14>, ReverbInfo::MaxDelayLines> mix_matrix{
            state.prev_feedback_output[2] + state.prev_feedback_output[1] + pre_delay_sample,
            -state.prev_feedback_output[0] - state.prev_feedback_output[3] + pre_delay_sample,
            state.prev_feedback_output[0] - state.prev_feedback_output[3] + pre_delay_sample,
            state.prev_feedback_output[1] - state.prev_feedback_output[2] + pre_delay_sample,
        };

        std::array<Common::FixedPoint<50, 14>, ReverbInfo::MaxDelayLines> allpass_samples{};
        for (u32 i = 0; i < ReverbInfo::MaxDelayLines; i++) {
            allpass_samples[i] = Axfx2AllPassTick(state.decay_delay_lines[i],
                                                  state.fdn_delay_lines[i], mix_matrix[i]);
        }

        const auto dry_gain{Common::FixedPoint<50, 14>::from_base(params.dry_gain)};
        const auto wet_gain{Common::FixedPoint<50, 14>::from_base(params.wet_gain)};

        if constexpr (NumChannels == 6) {
            const std::array<Common::FixedPoint<50, 14>, MaxChannels> allpass_outputs{
                allpass_samples[0], allpass_samples[1], allpass_samples[2] - allpass_samples[3],
                allpass_samples[3], allpass_samples[2], allpass_samples[3],
            };

            for (u32 channel = 0; channel < NumChannels; channel++) {
                auto in_sample{inputs[channel][sample_index] * dry_gain};

                Common::FixedPoint<50, 14> allpass{};
                if (channel == static_cast<u32>(Channels::Center)) {
                    allpass = state.center_delay_line.Tick(allpass_outputs[channel] * 0.5f);
                } else {
                    allpass = allpass_outputs[channel];
                }

                auto out_sample{((output_samples[channel] + allpass) * wet_gain) / 64};
                outputs[channel][sample_index] = (in_sample + out_sample).to_int();
            }
        } else {
            for (u32 channel = 0; channel < NumChannels; channel++) {
                auto in_sample{inputs[channel][sample_index] * dry_gain};
                auto out_sample{((output_samples[channel] + allpass_samples[channel]) * wet_gain) /
                                64};
                outputs[channel][sample_index] = (in_sample + out_sample).to_int();
            }
        }
    }
}

/**
 * Apply a Reverb if enabled, according to the current state, on the input mix buffers,
 * saving the results to the output mix buffers.
 *
 * @param params       - Input parameters to use.
 * @param state        - State to use, must be initialized (see InitializeReverbEffect).
 * @param enabled      - If enabled, delay will be applied, otherwise input is copied to output.
 * @param inputs       - Input mix buffers to performan the reverb on.
 * @param outputs      - Output mix buffers to receive the reverbed samples.
 * @param sample_count - Number of samples to process.
 */
static void ApplyReverbEffect(const ReverbInfo::ParameterVersion2& params, ReverbInfo::State& state,
                              const bool enabled, std::vector<std::span<const s32>>& inputs,
                              std::vector<std::span<s32>>& outputs, const u32 sample_count) {
    if (enabled) {
        switch (params.channel_count) {
        case 0:
            return;
        case 1:
            ApplyReverbEffect<1>(params, state, inputs, outputs, sample_count);
            break;
        case 2:
            ApplyReverbEffect<2>(params, state, inputs, outputs, sample_count);
            break;
        case 4:
            ApplyReverbEffect<4>(params, state, inputs, outputs, sample_count);
            break;
        case 6:
            ApplyReverbEffect<6>(params, state, inputs, outputs, sample_count);
            break;
        default:
            ApplyReverbEffectBypass(inputs, outputs, params.channel_count, sample_count);
            break;
        }
    } else {
        ApplyReverbEffectBypass(inputs, outputs, params.channel_count, sample_count);
    }
}

void ReverbCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                         std::string& string) {
    string += fmt::format(
        "ReverbCommand\n\tenabled {} long_size_pre_delay_supported {}\n\tinputs: ", effect_enabled,
        long_size_pre_delay_supported);
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n\toutputs: ";
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", outputs[i]);
    }
    string += "\n";
}

void ReverbCommand::Process(const ADSP::CommandListProcessor& processor) {
    std::vector<std::span<const s32>> input_buffers(parameter.channel_count);
    std::vector<std::span<s32>> output_buffers(parameter.channel_count);

    for (u32 i = 0; i < parameter.channel_count; i++) {
        input_buffers[i] = processor.mix_buffers.subspan(inputs[i] * processor.sample_count,
                                                         processor.sample_count);
        output_buffers[i] = processor.mix_buffers.subspan(outputs[i] * processor.sample_count,
                                                          processor.sample_count);
    }

    auto state_{reinterpret_cast<ReverbInfo::State*>(state)};

    if (effect_enabled) {
        if (parameter.state == ReverbInfo::ParameterState::Updating) {
            UpdateReverbEffectParameter(parameter, *state_);
        } else if (parameter.state == ReverbInfo::ParameterState::Initialized) {
            InitializeReverbEffect(parameter, *state_, workbuffer, long_size_pre_delay_supported);
        }
    }
    ApplyReverbEffect(parameter, *state_, effect_enabled, input_buffers, output_buffers,
                      processor.sample_count);
}

bool ReverbCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
