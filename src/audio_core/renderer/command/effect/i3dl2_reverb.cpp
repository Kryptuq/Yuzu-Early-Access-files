// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <numbers>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/i3dl2_reverb.h"

namespace AudioCore::AudioRenderer {

constexpr std::array<f32, I3dl2ReverbInfo::MaxDelayLines> MinDelayLineTimes{
    5.0f,
    6.0f,
    13.0f,
    14.0f,
};
constexpr std::array<f32, I3dl2ReverbInfo::MaxDelayLines> MaxDelayLineTimes{
    45.7042007446f,
    82.7817001343f,
    149.938293457f,
    271.575805664f,
};
constexpr std::array<f32, I3dl2ReverbInfo::MaxDelayLines> Decay0MaxDelayLineTimes{17.0f, 13.0f,
                                                                                  9.0f, 7.0f};
constexpr std::array<f32, I3dl2ReverbInfo::MaxDelayLines> Decay1MaxDelayLineTimes{19.0f, 11.0f,
                                                                                  10.0f, 6.0f};
constexpr std::array<f32, I3dl2ReverbInfo::MaxDelayTaps> EarlyTapTimes{
    0.0171360000968f,
    0.0591540001333f,
    0.161733001471f,
    0.390186011791f,
    0.425262004137f,
    0.455410987139f,
    0.689737021923f,
    0.74590998888f,
    0.833844006062f,
    0.859502017498f,
    0.0f,
    0.0750240013003f,
    0.168788000941f,
    0.299901008606f,
    0.337442994118f,
    0.371903002262f,
    0.599011003971f,
    0.716741025448f,
    0.817858994007f,
    0.85166400671f,
};

constexpr std::array<f32, I3dl2ReverbInfo::MaxDelayTaps> EarlyGains{
    0.67096f, 0.61027f, 1.0f,     0.3568f,  0.68361f, 0.65978f, 0.51939f,
    0.24712f, 0.45945f, 0.45021f, 0.64196f, 0.54879f, 0.92925f, 0.3827f,
    0.72867f, 0.69794f, 0.5464f,  0.24563f, 0.45214f, 0.44042f};

/**
 * Update the I3dl2ReverbInfo state according to the given parameters.
 *
 * @param params - Input parameters to update the state.
 * @param state  - State to be updated.
 * @param reset  - If enabled, the state buffers will be reset. Only set this on initialize.
 */
static void UpdateI3dl2ReverbEffectParameter(const I3dl2ReverbInfo::ParameterVersion1& params,
                                             I3dl2ReverbInfo::State& state, const bool reset) {
    const auto pow_10 = [](f32 val) -> f32 {
        return (val >= 0.0f) ? 1.0f : (val <= -5.3f) ? 0.0f : std::pow(10.0f, val);
    };
    const auto sin = [](f32 degrees) -> f32 {
        return std::sin(degrees * std::numbers::pi_v<f32> / 180.0f);
    };
    const auto cos = [](f32 degrees) -> f32 {
        return std::cos(degrees * std::numbers::pi_v<f32> / 180.0f);
    };

    Common::FixedPoint<50, 14> delay{static_cast<f32>(params.sample_rate) / 1000.0f};

    state.dry_gain = params.dry_gain;
    Common::FixedPoint<50, 14> early_gain{
        std::min(params.room_gain + params.reflection_gain, 5000.0f) / 2000.0f};
    state.early_gain = pow_10(early_gain.to_float());
    Common::FixedPoint<50, 14> late_gain{std::min(params.room_gain + params.reverb_gain, 5000.0f) /
                                         2000.0f};
    state.late_gain = pow_10(late_gain.to_float());

    Common::FixedPoint<50, 14> hf_gain{pow_10(params.room_HF_gain / 2000.0f)};
    if (hf_gain >= 1.0f) {
        state.lowpass_1 = 0.0f;
        state.lowpass_2 = 1.0f;
    } else {
        const auto reference_hf{(params.reference_HF * 256.0f) /
                                static_cast<f32>(params.sample_rate)};
        const Common::FixedPoint<50, 14> a{1.0f - hf_gain.to_float()};
        const Common::FixedPoint<50, 14> b{2.0f + (-cos(reference_hf) * (hf_gain * 2.0f))};
        const Common::FixedPoint<50, 14> c{
            std::sqrt(std::pow(b.to_float(), 2.0f) + (std::pow(a.to_float(), 2.0f) * -4.0f))};

        state.lowpass_1 = std::min(((b - c) / (a * 2.0f)).to_float(), 0.99723f);
        state.lowpass_2 = 1.0f - state.lowpass_1;
    }

    state.early_to_late_taps =
        (((params.reflection_delay + params.late_reverb_delay_time) * 1000.0f) * delay).to_int();
    state.last_reverb_echo = params.late_reverb_diffusion * 0.6f * 0.01f;

    for (u32 i = 0; i < I3dl2ReverbInfo::MaxDelayLines; i++) {
        auto curr_delay{
            ((MinDelayLineTimes[i] + (params.late_reverb_density / 100.0f) *
                                         (MaxDelayLineTimes[i] - MinDelayLineTimes[i])) *
             delay)
                .to_int()};
        state.fdn_delay_lines[i].SetDelay(curr_delay);

        const auto a{
            (static_cast<f32>(state.fdn_delay_lines[i].delay + state.decay_delay_lines0[i].delay +
                              state.decay_delay_lines1[i].delay) *
             -60.0f) /
            (params.late_reverb_decay_time * static_cast<f32>(params.sample_rate))};
        const auto b{a / params.late_reverb_HF_decay_ratio};
        const auto c{
            cos(((params.reference_HF * 0.5f) * 128.0f) / static_cast<f32>(params.sample_rate)) /
            sin(((params.reference_HF * 0.5f) * 128.0f) / static_cast<f32>(params.sample_rate))};
        const auto d{pow_10((b - a) / 40.0f)};
        const auto e{pow_10((b + a) / 40.0f) * 0.7071f};

        state.lowpass_coeff[i][0] = ((c * d + 1.0f) * e) / (c + d);
        state.lowpass_coeff[i][1] = ((1.0f - (c * d)) * e) / (c + d);
        state.lowpass_coeff[i][2] = (c - d) / (c + d);

        state.decay_delay_lines0[i].wet_gain = state.last_reverb_echo;
        state.decay_delay_lines1[i].wet_gain = state.last_reverb_echo * -0.9f;
    }

    if (reset) {
        state.shelf_filter.fill(0.0f);
        state.lowpass_0 = 0.0f;
        for (u32 i = 0; i < I3dl2ReverbInfo::MaxDelayLines; i++) {
            std::ranges::fill(state.fdn_delay_lines[i].buffer, 0);
            std::ranges::fill(state.decay_delay_lines0[i].buffer, 0);
            std::ranges::fill(state.decay_delay_lines1[i].buffer, 0);
        }
        std::ranges::fill(state.center_delay_line.buffer, 0);
        std::ranges::fill(state.early_delay_line.buffer, 0);
    }

    const auto reflection_time{(params.late_reverb_delay_time * 0.9998f + 0.02f) * 1000.0f};
    const auto reflection_delay{params.reflection_delay * 1000.0f};
    for (u32 i = 0; i < I3dl2ReverbInfo::MaxDelayTaps; i++) {
        auto length{((reflection_delay + reflection_time * EarlyTapTimes[i]) * delay).to_int()};
        if (length >= state.early_delay_line.max_delay) {
            length = state.early_delay_line.max_delay;
        }
        state.early_tap_steps[i] = length;
    }
}

/**
 * Initialize a new I3dl2ReverbInfo state according to the given parameters.
 *
 * @param params     - Input parameters to update the state.
 * @param state      - State to be updated.
 * @param workbuffer - Game-supplied memory for the state. (Unused)
 */
static void InitializeI3dl2ReverbEffect(const I3dl2ReverbInfo::ParameterVersion1& params,
                                        I3dl2ReverbInfo::State& state, const CpuAddr workbuffer) {
    state = {};
    Common::FixedPoint<50, 14> delay{static_cast<f32>(params.sample_rate) / 1000};

    for (u32 i = 0; i < I3dl2ReverbInfo::MaxDelayLines; i++) {
        auto fdn_delay_time{(MaxDelayLineTimes[i] * delay).to_uint_floor()};
        state.fdn_delay_lines[i].Initialize(fdn_delay_time);

        auto decay0_delay_time{(Decay0MaxDelayLineTimes[i] * delay).to_uint_floor()};
        state.decay_delay_lines0[i].Initialize(decay0_delay_time);

        auto decay1_delay_time{(Decay1MaxDelayLineTimes[i] * delay).to_uint_floor()};
        state.decay_delay_lines1[i].Initialize(decay1_delay_time);
    }

    const auto center_delay_time{(5 * delay).to_uint_floor()};
    state.center_delay_line.Initialize(center_delay_time);

    const auto early_delay_time{(400 * delay).to_uint_floor()};
    state.early_delay_line.Initialize(early_delay_time);

    UpdateI3dl2ReverbEffectParameter(params, state, true);
}

/**
 * Pass-through the effect, copying input to output directly, with no reverb applied.
 *
 * @param inputs        - Array of input mix buffers to copy.
 * @param outputs       - Array of output mix buffers to receive copy.
 * @param channel_count - Number of channels in inputs and outputs.
 * @param sample_count  - Number of samples within each channel (unused).
 */
static void ApplyI3dl2ReverbEffectBypass(std::span<std::span<const s32>> inputs,
                                         std::span<std::span<s32>> outputs, const u32 channel_count,
                                         [[maybe_unused]] const u32 sample_count) {
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
 * @param decay0 - The first decay line.
 * @param decay1 - The second decay line.
 * @param fdn    - Feedback delay network.
 * @param mix    - The new calculated sample to be written and decayed.
 * @return The next delayed and decayed sample.
 */
static Common::FixedPoint<50, 14> Axfx2AllPassTick(I3dl2ReverbInfo::I3dl2DelayLine& decay0,
                                                   I3dl2ReverbInfo::I3dl2DelayLine& decay1,
                                                   I3dl2ReverbInfo::I3dl2DelayLine& fdn,
                                                   const Common::FixedPoint<50, 14> mix) {
    auto val{decay0.Read()};
    auto mixed{mix - (val * decay0.wet_gain)};
    auto out{decay0.Tick(mixed) + (mixed * decay0.wet_gain)};

    val = decay1.Read();
    mixed = out - (val * decay1.wet_gain);
    out = decay1.Tick(mixed) + (mixed * decay1.wet_gain);

    fdn.Tick(out);
    return out;
}

/**
 * Impl. Apply a I3DL2 reverb according to the current state, on the input mix buffers,
 * saving the results to the output mix buffers.
 *
 * @tparam Channels    - Number of channels to process. 1-6.
                         Inputs/outputs should have this many buffers.
 * @param state        - State to use, must be initialized (see InitializeI3dl2ReverbEffect).
 * @param inputs       - Input mix buffers to perform the reverb on.
 * @param outputs      - Output mix buffers to receive the reverbed samples.
 * @param sample_count - Number of samples to process.
 */
template <size_t Channels>
static void ApplyI3dl2ReverbEffect(I3dl2ReverbInfo::State& state,
                                   std::span<std::span<const s32>> inputs,
                                   std::span<std::span<s32>> outputs, const u32 sample_count) {
    constexpr std::array<u8, I3dl2ReverbInfo::MaxDelayTaps> OutTapIndexes1Ch{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    constexpr std::array<u8, I3dl2ReverbInfo::MaxDelayTaps> OutTapIndexes2Ch{
        0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
    };
    constexpr std::array<u8, I3dl2ReverbInfo::MaxDelayTaps> OutTapIndexes4Ch{
        0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 3, 3, 3,
    };
    constexpr std::array<u8, I3dl2ReverbInfo::MaxDelayTaps> OutTapIndexes6Ch{
        4, 0, 0, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
    };

    std::span<const u8> tap_indexes{};
    if constexpr (Channels == 1) {
        tap_indexes = OutTapIndexes1Ch;
    } else if constexpr (Channels == 2) {
        tap_indexes = OutTapIndexes2Ch;
    } else if constexpr (Channels == 4) {
        tap_indexes = OutTapIndexes4Ch;
    } else if constexpr (Channels == 6) {
        tap_indexes = OutTapIndexes6Ch;
    }

    for (u32 i = 0; i < sample_count; i++) {
        Common::FixedPoint<50, 14> early_to_late_tap{
            state.early_delay_line.TapOut(state.early_to_late_taps)};
        std::array<Common::FixedPoint<50, 14>, Channels> output_samples{};

        for (u32 early_tap = 0; early_tap < I3dl2ReverbInfo::MaxDelayTaps; early_tap++) {
            output_samples[tap_indexes[early_tap]] +=
                state.early_delay_line.TapOut(state.early_tap_steps[early_tap]) *
                EarlyGains[early_tap];
            if constexpr (Channels == 6) {
                output_samples[5] +=
                    state.early_delay_line.TapOut(state.early_tap_steps[early_tap]) *
                    EarlyGains[early_tap];
            }
        }

        Common::FixedPoint<50, 14> current_sample{};
        for (u32 channel = 0; channel < Channels; channel++) {
            current_sample += inputs[channel][i];
        }

        state.lowpass_0 =
            (current_sample * state.lowpass_2 + state.lowpass_0 * state.lowpass_1).to_float();
        state.early_delay_line.Tick(state.lowpass_0);

        for (u32 channel = 0; channel < Channels; channel++) {
            output_samples[channel] *= state.early_gain;
        }

        std::array<Common::FixedPoint<50, 14>, I3dl2ReverbInfo::MaxDelayLines> filtered_samples{};
        for (u32 delay_line = 0; delay_line < I3dl2ReverbInfo::MaxDelayLines; delay_line++) {
            filtered_samples[delay_line] =
                state.fdn_delay_lines[delay_line].Read() * state.lowpass_coeff[delay_line][0] +
                state.shelf_filter[delay_line];
            state.shelf_filter[delay_line] =
                (filtered_samples[delay_line] * state.lowpass_coeff[delay_line][2] +
                 state.fdn_delay_lines[delay_line].Read() * state.lowpass_coeff[delay_line][1])
                    .to_float();
        }

        const std::array<Common::FixedPoint<50, 14>, I3dl2ReverbInfo::MaxDelayLines> mix_matrix{
            filtered_samples[1] + filtered_samples[2] + early_to_late_tap * state.late_gain,
            -filtered_samples[0] - filtered_samples[3] + early_to_late_tap * state.late_gain,
            filtered_samples[0] - filtered_samples[3] + early_to_late_tap * state.late_gain,
            filtered_samples[1] - filtered_samples[2] + early_to_late_tap * state.late_gain,
        };

        std::array<Common::FixedPoint<50, 14>, I3dl2ReverbInfo::MaxDelayLines> allpass_samples{};
        for (u32 delay_line = 0; delay_line < I3dl2ReverbInfo::MaxDelayLines; delay_line++) {
            allpass_samples[delay_line] = Axfx2AllPassTick(
                state.decay_delay_lines0[delay_line], state.decay_delay_lines1[delay_line],
                state.fdn_delay_lines[delay_line], mix_matrix[delay_line]);
        }

        const auto out_channels{std::min(Channels, size_t(4))};
        for (u32 channel = 0; channel < out_channels; channel++) {
            auto out_sample{output_samples[channel] + allpass_samples[channel] +
                            state.dry_gain * static_cast<f32>(inputs[channel][i])};
            outputs[channel][i] =
                static_cast<s32>(std::clamp(out_sample.to_float(), -8388600.0f, 8388600.0f));
        }

        if constexpr (Channels == 6) {
            auto center{
                state.center_delay_line.Tick((allpass_samples[2] - allpass_samples[3]) * 0.5f)};
            auto out_sample{static_cast<f32>(inputs[4][i]) * state.dry_gain +
                            output_samples[4] * state.early_gain + center};
            outputs[4][i] =
                static_cast<s32>(std::clamp(out_sample.to_float(), -8388600.0f, 8388600.0f));

            out_sample = static_cast<f32>(inputs[5][i]) * state.dry_gain +
                         output_samples[5] * state.early_gain + allpass_samples[3];
            outputs[5][i] =
                static_cast<s32>(std::clamp(out_sample.to_float(), -8388600.0f, 8388600.0f));
        }
    }
}

/**
 * Apply a I3DL2 reverb if enabled, according to the current state, on the input mix buffers,
 * saving the results to the output mix buffers.
 *
 * @param params       - Input parameters to use.
 * @param state        - State to use, must be initialized (see InitializeI3dl2ReverbEffect).
 * @param enabled      - If enabled, delay will be applied, otherwise input is copied to output.
 * @param inputs       - Input mix buffers to performan the delay on.
 * @param outputs      - Output mix buffers to receive the delayed samples.
 * @param sample_count - Number of samples to process.
 */
static void ApplyI3dl2ReverbEffect(const I3dl2ReverbInfo::ParameterVersion1& params,
                                   I3dl2ReverbInfo::State& state, const bool enabled,
                                   std::span<std::span<const s32>> inputs,
                                   std::span<std::span<s32>> outputs, const u32 sample_count) {
    if (enabled) {
        switch (params.channel_count) {
        case 0:
            return;
        case 1:
            ApplyI3dl2ReverbEffect<1>(state, inputs, outputs, sample_count);
            break;
        case 2:
            ApplyI3dl2ReverbEffect<2>(state, inputs, outputs, sample_count);
            break;
        case 4:
            ApplyI3dl2ReverbEffect<4>(state, inputs, outputs, sample_count);
            break;
        case 6:
            ApplyI3dl2ReverbEffect<6>(state, inputs, outputs, sample_count);
            break;
        default:
            ApplyI3dl2ReverbEffectBypass(inputs, outputs, params.channel_count, sample_count);
            break;
        }
    } else {
        ApplyI3dl2ReverbEffectBypass(inputs, outputs, params.channel_count, sample_count);
    }
}

void I3dl2ReverbCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                              std::string& string) {
    string += fmt::format("I3dl2ReverbCommand\n\tenabled {} \n\tinputs: ", effect_enabled);
    for (u32 i = 0; i < parameter.channel_count; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n\toutputs: ";
    for (u32 i = 0; i < parameter.channel_count; i++) {
        string += fmt::format("{:02X}, ", outputs[i]);
    }
    string += "\n";
}

void I3dl2ReverbCommand::Process(const ADSP::CommandListProcessor& processor) {
    std::vector<std::span<const s32>> input_buffers(parameter.channel_count);
    std::vector<std::span<s32>> output_buffers(parameter.channel_count);

    for (u32 i = 0; i < parameter.channel_count; i++) {
        input_buffers[i] = processor.mix_buffers.subspan(inputs[i] * processor.sample_count,
                                                         processor.sample_count);
        output_buffers[i] = processor.mix_buffers.subspan(outputs[i] * processor.sample_count,
                                                          processor.sample_count);
    }

    auto state_{reinterpret_cast<I3dl2ReverbInfo::State*>(state)};

    if (effect_enabled) {
        if (parameter.state == I3dl2ReverbInfo::ParameterState::Updating) {
            UpdateI3dl2ReverbEffectParameter(parameter, *state_, false);
        } else if (parameter.state == I3dl2ReverbInfo::ParameterState::Initialized) {
            InitializeI3dl2ReverbEffect(parameter, *state_, workbuffer);
        }
    }
    ApplyI3dl2ReverbEffect(parameter, *state_, effect_enabled, input_buffers, output_buffers,
                           processor.sample_count);
}

bool I3dl2ReverbCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
