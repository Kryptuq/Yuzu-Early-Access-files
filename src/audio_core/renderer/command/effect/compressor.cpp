// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <span>
#include <vector>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/compressor.h"
#include "audio_core/renderer/effect/compressor.h"
#include "common/scratch_buffer.h"

namespace AudioCore::AudioRenderer {

static void SetCompressorEffectParameter(const CompressorInfo::ParameterVersion2& params,
                                         CompressorInfo::State& state) {
    const auto ratio{1.0f / params.compressor_ratio};
    auto makeup_gain{0.0f};
    if (params.makeup_gain_enabled) {
        makeup_gain = (params.threshold * 0.5f) * (ratio - 1.0f) - 3.0f;
    }
    state.makeup_gain = makeup_gain;
    state.unk_18 = params.unk_28;

    const auto a{(params.out_gain + makeup_gain) / 20.0f * 3.3219f};
    const auto b{(a - std::trunc(a)) * 0.69315f};
    const auto c{std::pow(2.0f, b)};

    state.unk_0C = (1.0f - ratio) / 6.0f;
    state.unk_14 = params.threshold + 1.5f;
    state.unk_10 = params.threshold - 1.5f;
    state.unk_20 = c;
}

static void InitializeCompressorEffect(const CompressorInfo::ParameterVersion2& params,
                                       CompressorInfo::State& state) {
    state = {};

    state.unk_00 = 0;
    state.unk_04 = 1.0f;
    state.unk_08 = 1.0f;

    SetCompressorEffectParameter(params, state);
}

static void ApplyCompressorEffect(const CompressorInfo::ParameterVersion2& params,
                                  CompressorInfo::State& state, bool enabled,
                                  std::span<std::span<const s32>> input_buffers,
                                  std::span<std::span<s32>> output_buffers, u32 sample_count) {
    if (enabled) {
        auto state_00{state.unk_00};
        auto state_04{state.unk_04};
        auto state_08{state.unk_08};
        auto state_18{state.unk_18};

        for (u32 i = 0; i < sample_count; i++) {
            auto a{0.0f};
            for (s16 channel = 0; channel < params.channel_count; channel++) {
                const auto input_sample{Common::FixedPoint<49, 15>(input_buffers[channel][i])};
                a += (input_sample * input_sample).to_float();
            }

            state_00 += params.unk_24 * ((a / params.channel_count) - state.unk_00);

            auto b{-100.0f};
            auto c{0.0f};
            if (state_00 >= 1.0e-10) {
                b = std::log10(state_00) * 10.0f;
                c = 1.0f;
            }

            if (b >= state.unk_10) {
                const auto d{b >= state.unk_14
                                 ? ((1.0f / params.compressor_ratio) - 1.0f) *
                                       (b - params.threshold)
                                 : (b - state.unk_10) * (b - state.unk_10) * -state.unk_0C};
                const auto e{d / 20.0f * 3.3219f};
                const auto f{(e - std::trunc(e)) * 0.69315f};
                c = std::pow(2.0f, f);
            }

            state_18 = params.unk_28;
            auto tmp{c};
            if ((state_04 - c) <= 0.08f) {
                state_18 = params.unk_2C;
                if (((state_04 - c) >= -0.08f) && (std::abs(state_08 - c) >= 0.001f)) {
                    tmp = state_04;
                }
            }

            state_04 = tmp;
            state_08 += (c - state_08) * state_18;

            for (s16 channel = 0; channel < params.channel_count; channel++) {
                output_buffers[channel][i] = static_cast<s32>(
                    static_cast<f32>(input_buffers[channel][i]) * state_08 * state.unk_20);
            }
        }

        state.unk_00 = state_00;
        state.unk_04 = state_04;
        state.unk_08 = state_08;
        state.unk_18 = state_18;
    } else {
        for (s16 channel = 0; channel < params.channel_count; channel++) {
            if (params.inputs[channel] != params.outputs[channel]) {
                std::memcpy(output_buffers[channel].data(), input_buffers[channel].data(),
                            output_buffers[channel].size_bytes());
            }
        }
    }
}

void CompressorCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                             std::string& string) {
    string += fmt::format("CompressorCommand\n\tenabled {} \n\tinputs: ", effect_enabled);
    for (s16 i = 0; i < parameter.channel_count; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n\toutputs: ";
    for (s16 i = 0; i < parameter.channel_count; i++) {
        string += fmt::format("{:02X}, ", outputs[i]);
    }
    string += "\n";
}

void CompressorCommand::Process(const ADSP::CommandListProcessor& processor) {
    static Common::ScratchBuffer<std::span<const s32>> input_buffers{};
    static Common::ScratchBuffer<std::span<s32>> output_buffers{};
    input_buffers.resize_destructive(parameter.channel_count);
    output_buffers.resize_destructive(parameter.channel_count);

    for (s16 i = 0; i < parameter.channel_count; i++) {
        input_buffers[i] = processor.mix_buffers.subspan(inputs[i] * processor.sample_count,
                                                         processor.sample_count);
        output_buffers[i] = processor.mix_buffers.subspan(outputs[i] * processor.sample_count,
                                                          processor.sample_count);
    }

    auto state_{reinterpret_cast<CompressorInfo::State*>(state)};

    if (effect_enabled) {
        if (parameter.state == CompressorInfo::ParameterState::Updating) {
            SetCompressorEffectParameter(parameter, *state_);
        } else if (parameter.state == CompressorInfo::ParameterState::Initialized) {
            InitializeCompressorEffect(parameter, *state_);
        }
    }

    ApplyCompressorEffect(parameter, *state_, effect_enabled, input_buffers, output_buffers,
                          processor.sample_count);
}

bool CompressorCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
