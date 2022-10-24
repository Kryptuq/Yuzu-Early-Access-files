// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/biquad_filter.h"
#include "audio_core/renderer/voice/voice_state.h"

namespace AudioCore::AudioRenderer {
/**
 * Biquad filter float implementation.
 *
 * @param output       - Output container for filtered samples.
 * @param input        - Input container for samples to be filtered.
 * @param b            - Feedforward coefficients.
 * @param a            - Feedback coefficients.
 * @param state        - State to track previous samples between calls.
 * @param sample_count - Number of samples to process.
 */
void ApplyBiquadFilterFloat(std::span<s32> output, std::span<const s32> input,
                            std::array<s16, 3>& b_, std::array<s16, 2>& a_,
                            VoiceState::BiquadFilterState& state, const u32 sample_count) {
    constexpr s64 min{std::numeric_limits<s32>::min()};
    constexpr s64 max{std::numeric_limits<s32>::max()};
    std::array<f64, 3> b{Common::FixedPoint<50, 14>::from_base(b_[0]).to_double(),
                         Common::FixedPoint<50, 14>::from_base(b_[1]).to_double(),
                         Common::FixedPoint<50, 14>::from_base(b_[2]).to_double()};
    std::array<f64, 2> a{Common::FixedPoint<50, 14>::from_base(a_[0]).to_double(),
                         Common::FixedPoint<50, 14>::from_base(a_[1]).to_double()};
    std::array<f64, 4> s{state.s0.to_double(), state.s1.to_double(), state.s2.to_double(),
                         state.s3.to_double()};

    for (u32 i = 0; i < sample_count; i++) {
        f64 in_sample{static_cast<f64>(input[i])};
        auto sample{in_sample * b[0] + s[0] * b[1] + s[1] * b[2] + s[2] * a[0] + s[3] * a[1]};

        output[i] = static_cast<s32>(std::clamp(static_cast<s64>(sample), min, max));

        s[1] = s[0];
        s[0] = in_sample;
        s[3] = s[2];
        s[2] = sample;
    }

    state.s0 = s[0];
    state.s1 = s[1];
    state.s2 = s[2];
    state.s3 = s[3];
}

/**
 * Biquad filter s32 implementation.
 *
 * @param output       - Output container for filtered samples.
 * @param input        - Input container for samples to be filtered.
 * @param b            - Feedforward coefficients.
 * @param a            - Feedback coefficients.
 * @param state        - State to track previous samples between calls.
 * @param sample_count - Number of samples to process.
 */
static void ApplyBiquadFilterInt(std::span<s32> output, std::span<const s32> input,
                                 std::array<s16, 3>& b_, std::array<s16, 2>& a_,
                                 VoiceState::BiquadFilterState& state, const u32 sample_count) {
    constexpr s64 min{std::numeric_limits<s32>::min()};
    constexpr s64 max{std::numeric_limits<s32>::max()};
    std::array<Common::FixedPoint<50, 14>, 3> b{
        Common::FixedPoint<50, 14>::from_base(b_[0]),
        Common::FixedPoint<50, 14>::from_base(b_[1]),
        Common::FixedPoint<50, 14>::from_base(b_[2]),
    };
    std::array<Common::FixedPoint<50, 14>, 3> a{
        Common::FixedPoint<50, 14>::from_base(a_[0]),
        Common::FixedPoint<50, 14>::from_base(a_[1]),
    };

    for (u32 i = 0; i < sample_count; i++) {
        s64 in_sample{input[i]};
        auto sample{in_sample * b[0] + state.s0};
        const auto out_sample{std::clamp(sample.to_long(), min, max)};

        output[i] = static_cast<s32>(out_sample);

        state.s0 = state.s1 + b[1] * in_sample + a[0] * out_sample;
        state.s1 = 0 + b[2] * in_sample + a[1] * out_sample;
    }
}

void BiquadFilterCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                               std::string& string) {
    string += fmt::format(
        "BiquadFilterCommand\n\tinput {:02X} output {:02X} needs_init {} use_float_processing {}\n",
        input, output, needs_init, use_float_processing);
}

void BiquadFilterCommand::Process(const ADSP::CommandListProcessor& processor) {
    auto state_{reinterpret_cast<VoiceState::BiquadFilterState*>(state)};
    if (needs_init) {
        *state_ = {};
    }

    auto input_buffer{
        processor.mix_buffers.subspan(input * processor.sample_count, processor.sample_count)};
    auto output_buffer{
        processor.mix_buffers.subspan(output * processor.sample_count, processor.sample_count)};

    if (use_float_processing) {
        ApplyBiquadFilterFloat(output_buffer, input_buffer, biquad.b, biquad.a, *state_,
                               processor.sample_count);
    } else {
        ApplyBiquadFilterInt(output_buffer, input_buffer, biquad.b, biquad.a, *state_,
                             processor.sample_count);
    }
}

bool BiquadFilterCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
