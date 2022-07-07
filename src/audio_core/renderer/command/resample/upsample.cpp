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
    constexpr std::array<Common::FixedPoint<24, 8>, WindowSize> SincWindow1{
        51.93359375f, -18.80078125f, 9.73046875f, -5.33203125f, 2.84375f,
        -1.41015625f, 0.62109375f,   -0.2265625f, 0.0625f,      -0.00390625f,
    };
    constexpr std::array<Common::FixedPoint<24, 8>, WindowSize> SincWindow2{
        105.35546875f, -24.52734375f, 11.9609375f,  -6.515625f, 3.52734375f,
        -1.796875f,    0.828125f,     -0.32421875f, 0.1015625f, -0.015625f,
    };
    constexpr std::array<Common::FixedPoint<24, 8>, WindowSize> SincWindow3{
        122.08203125f, -16.47656250f, 7.68359375f,  -4.15625000f, 2.26171875f,
        -1.16796875f,  0.54687500f,   -0.22265625f, 0.07421875f,  -0.01171875f,
    };
    constexpr std::array<Common::FixedPoint<24, 8>, WindowSize> SincWindow4{
        23.73437500f, -9.62109375f, 5.07812500f,  -2.78125000f, 1.46875000f,
        -0.71484375f, 0.30859375f,  -0.10546875f, 0.02734375f,  0.00000000f,
    };
    constexpr std::array<Common::FixedPoint<24, 8>, WindowSize> SincWindow5{
        80.62500000f, -24.67187500f, 12.44921875f, -6.80859375f, 3.66406250f,
        -1.83984375f, 0.83203125f,   -0.31640625f, 0.09375000f,  -0.01171875f,
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

    auto calculate_sample = [&state](std::span<const Common::FixedPoint<24, 8>> coeffs1,
                                     std::span<const Common::FixedPoint<24, 8>> coeffs2) -> s32 {
        auto output_index{state->history_output_index};
        auto start_pos{output_index - state->history_start_index + 1U};
        auto end_pos{10U};

        if (start_pos < 10) {
            end_pos = start_pos;
        }

        u64 prev_contrib{0};
        u32 coeff_index{0};
        for (; coeff_index < end_pos; coeff_index++, output_index--) {
            prev_contrib += static_cast<u64>(state->history[output_index].to_raw()) *
                            coeffs1[coeff_index].to_raw();
        }

        auto end_index{state->history_end_index};
        for (; start_pos < 9; start_pos++, coeff_index++, end_index--) {
            prev_contrib += static_cast<u64>(state->history[end_index].to_raw()) *
                            coeffs1[coeff_index].to_raw();
        }

        output_index =
            static_cast<u16>((state->history_output_index + 1) % UpsamplerState::HistorySize);
        start_pos = state->history_end_index - output_index + 1U;
        end_pos = 10U;

        if (start_pos < 10) {
            end_pos = start_pos;
        }

        u64 next_contrib{0};
        coeff_index = 0;
        for (; coeff_index < end_pos; coeff_index++, output_index++) {
            next_contrib += static_cast<u64>(state->history[output_index].to_raw()) *
                            coeffs2[coeff_index].to_raw();
        }

        auto start_index{state->history_start_index};
        for (; start_pos < 9; start_pos++, start_index++, coeff_index++) {
            next_contrib += static_cast<u64>(state->history[start_index].to_raw()) *
                            coeffs2[coeff_index].to_raw();
        }

        return static_cast<s32>(((prev_contrib >> 15) + (next_contrib >> 15)) >> 8);
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
                output[write_index] = calculate_sample(SincWindow3, SincWindow4);
                break;

            case 2:
                output[write_index] = calculate_sample(SincWindow2, SincWindow1);
                break;

            case 3:
                output[write_index] = calculate_sample(SincWindow5, SincWindow5);
                break;

            case 4:
                output[write_index] = calculate_sample(SincWindow1, SincWindow2);
                break;

            case 5:
                output[write_index] = calculate_sample(SincWindow4, SincWindow3);
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
                output[write_index] = calculate_sample(SincWindow2, SincWindow1);
                break;

            case 2:
                output[write_index] = calculate_sample(SincWindow1, SincWindow2);
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
                output[write_index] = calculate_sample(SincWindow1, SincWindow2);
                break;

            case 2:
                increment();
                output[write_index] = calculate_sample(SincWindow2, SincWindow1);
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
