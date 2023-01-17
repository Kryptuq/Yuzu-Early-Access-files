// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/resample/upsample.h"
#include "audio_core/renderer/upsampler/upsampler_info.h"

namespace AudioCore::AudioRenderer {
/**
 * Upsampling impl. Input must be 8K, 16K or 32K, output is 48K.
 *
 * @param output              - Output buffer.
 * @param input               - Input buffer.
 * @param target_sample_count - Number of samples for output.
 * @param state               - Upsampler state, updated each call.
 */
static void SrcProcessFrame(std::span<s32> output, std::span<const s32> input,
                            const u32 target_sample_count, const u32 source_sample_count,
                            UpsamplerState* state) {
    constexpr u32 WindowSize = 10;
    constexpr std::array<Common::FixedPoint<17, 15>, WindowSize> WindowedSinc1{
        0.95376587f,   -0.12872314f, 0.060028076f,  -0.032470703f, 0.017669678f,
        -0.009124756f, 0.004272461f, -0.001739502f, 0.000579834f,  -0.000091552734f,
    };
    constexpr std::array<Common::FixedPoint<17, 15>, WindowSize> WindowedSinc2{
        0.8230896f,    -0.19161987f,  0.093444824f,  -0.05090332f,   0.027557373f,
        -0.014038086f, 0.0064697266f, -0.002532959f, 0.00079345703f, -0.00012207031f,
    };
    constexpr std::array<Common::FixedPoint<17, 15>, WindowSize> WindowedSinc3{
        0.6298828f,    -0.19274902f, 0.09725952f,    -0.05319214f,  0.028625488f,
        -0.014373779f, 0.006500244f, -0.0024719238f, 0.0007324219f, -0.000091552734f,
    };
    constexpr std::array<Common::FixedPoint<17, 15>, WindowSize> WindowedSinc4{
        0.4057312f,    -0.1468811f,  0.07601929f,    -0.041656494f,  0.022216797f,
        -0.011016846f, 0.004852295f, -0.0017700195f, 0.00048828125f, -0.000030517578f,
    };
    constexpr std::array<Common::FixedPoint<17, 15>, WindowSize> WindowedSinc5{
        0.1854248f,    -0.075164795f, 0.03967285f,    -0.021728516f,  0.011474609f,
        -0.005584717f, 0.0024108887f, -0.0008239746f, 0.00021362305f, 0.0f,
    };

    if (!state->initialized) {
        switch (source_sample_count) {
        case 40:
            state->window_size = WindowSize;
            state->ratio = 6.0f;
            state->history.fill(0);
            break;

        case 80:
            state->window_size = WindowSize;
            state->ratio = 3.0f;
            state->history.fill(0);
            break;

        case 160:
            state->window_size = WindowSize;
            state->ratio = 1.5f;
            state->history.fill(0);
            break;

        default:
            LOG_ERROR(Service_Audio, "Invalid upsampling source count {}!", source_sample_count);
            // This continues anyway, but let's assume 160 for sanity
            state->window_size = WindowSize;
            state->ratio = 1.5f;
            state->history.fill(0);
            break;
        }

        state->history_input_index = 0;
        state->history_output_index = 9;
        state->history_start_index = 0;
        state->history_end_index = UpsamplerState::HistorySize - 1;
        state->initialized = true;
    }

    if (target_sample_count == 0) {
        return;
    }

    u32 read_index{0};

    auto increment = [&]() -> void {
        state->history[state->history_input_index] = input[read_index++];
        state->history_input_index =
            static_cast<u16>((state->history_input_index + 1) % UpsamplerState::HistorySize);
        state->history_output_index =
            static_cast<u16>((state->history_output_index + 1) % UpsamplerState::HistorySize);
    };

    auto calculate_sample = [&state](std::span<const Common::FixedPoint<17, 15>> coeffs1,
                                     std::span<const Common::FixedPoint<17, 15>> coeffs2) -> s32 {
        auto output_index{state->history_output_index};
        u64 result{0};

        for (u32 coeff_index = 0; coeff_index < 10; coeff_index++) {
            result += static_cast<u64>(state->history[output_index].to_raw()) *
                      coeffs1[coeff_index].to_raw();

            output_index = output_index == state->history_start_index ? state->history_end_index
                                                                      : output_index - 1;
        }

        output_index =
            static_cast<u16>((state->history_output_index + 1) % UpsamplerState::HistorySize);

        for (u32 coeff_index = 0; coeff_index < 10; coeff_index++) {
            result += static_cast<u64>(state->history[output_index].to_raw()) *
                      coeffs2[coeff_index].to_raw();

            output_index = output_index == state->history_end_index ? state->history_start_index
                                                                    : output_index + 1;
        }

        return static_cast<s32>(result >> (8 + 15));
    };

    switch (state->ratio.to_int_floor()) {
    // 40 -> 240
    case 6:
        for (u32 write_index = 0; write_index < target_sample_count; write_index++) {
            switch (state->sample_index) {
            case 0:
                increment();
                output[write_index] = state->history[state->history_output_index].to_int_floor();
                break;

            case 1:
                output[write_index] = calculate_sample(WindowedSinc1, WindowedSinc5);
                break;

            case 2:
                output[write_index] = calculate_sample(WindowedSinc2, WindowedSinc4);
                break;

            case 3:
                output[write_index] = calculate_sample(WindowedSinc3, WindowedSinc3);
                break;

            case 4:
                output[write_index] = calculate_sample(WindowedSinc4, WindowedSinc2);
                break;

            case 5:
                output[write_index] = calculate_sample(WindowedSinc5, WindowedSinc1);
                break;
            }
            state->sample_index = static_cast<u8>((state->sample_index + 1) % 6);
        }
        break;

    // 80 -> 240
    case 3:
        for (u32 write_index = 0; write_index < target_sample_count; write_index++) {
            switch (state->sample_index) {
            case 0:
                increment();
                output[write_index] = state->history[state->history_output_index].to_int_floor();
                break;

            case 1:
                output[write_index] = calculate_sample(WindowedSinc2, WindowedSinc4);
                break;

            case 2:
                output[write_index] = calculate_sample(WindowedSinc4, WindowedSinc2);
                break;
            }
            state->sample_index = static_cast<u8>((state->sample_index + 1) % 3);
        }
        break;

    // 160 -> 240
    default:
        for (u32 write_index = 0; write_index < target_sample_count; write_index++) {
            switch (state->sample_index) {
            case 0:
                increment();
                output[write_index] = state->history[state->history_output_index].to_int_floor();
                break;

            case 1:
                output[write_index] = calculate_sample(WindowedSinc4, WindowedSinc2);
                break;

            case 2:
                increment();
                output[write_index] = calculate_sample(WindowedSinc2, WindowedSinc4);
                break;
            }
            state->sample_index = static_cast<u8>((state->sample_index + 1) % 3);
        }

        break;
    }
}

auto UpsampleCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                           std::string& string) -> void {
    string += fmt::format("UpsampleCommand\n\tsource_sample_count {} source_sample_rate {}",
                          source_sample_count, source_sample_rate);
    const auto upsampler{reinterpret_cast<UpsamplerInfo*>(upsampler_info)};
    if (upsampler != nullptr) {
        string += fmt::format("\n\tUpsampler\n\t\tenabled {} sample count {}\n\tinputs: ",
                              upsampler->enabled, upsampler->sample_count);
        for (u32 i = 0; i < upsampler->input_count; i++) {
            string += fmt::format("{:02X}, ", upsampler->inputs[i]);
        }
    }
    string += "\n";
}

void UpsampleCommand::Process(const ADSP::CommandListProcessor& processor) {
    const auto info{reinterpret_cast<UpsamplerInfo*>(upsampler_info)};
    const auto input_count{std::min(info->input_count, buffer_count)};
    const std::span<const s16> inputs_{reinterpret_cast<const s16*>(inputs), input_count};

    for (u32 i = 0; i < input_count; i++) {
        const auto channel{inputs_[i]};

        if (channel >= 0 && channel < static_cast<s16>(processor.buffer_count)) {
            auto state{&info->states[i]};
            std::span<s32> output{
                reinterpret_cast<s32*>(samples_buffer + info->sample_count * channel * sizeof(s32)),
                info->sample_count};
            auto input{processor.mix_buffers.subspan(channel * processor.sample_count,
                                                     processor.sample_count)};

            SrcProcessFrame(output, input, info->sample_count, source_sample_count, state);
        }
    }
}

bool UpsampleCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
