// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "audio_core/codec.h"

namespace AudioCore::Codec {

std::vector<s16> DecodeADPCM(const u8* const data, std::size_t size, const ADPCM_Coeff& coeff,
                             ADPCMState& state) {
    // GC-ADPCM with scale factor and variable coefficients.
    // Frames are 8 bytes long containing 14 samples each.
    // Samples are 4 bits (one nibble) long.

    constexpr std::size_t FRAME_LEN = 8;
    constexpr std::size_t SAMPLES_PER_FRAME = 14;
    static constexpr std::array<int, 16> SIGNED_NIBBLES{
        0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1,
    };

    const std::size_t sample_count = (size / FRAME_LEN) * SAMPLES_PER_FRAME;
    const std::size_t ret_size =
        sample_count % 2 == 0 ? sample_count : sample_count + 1; // Ensure multiple of two.
    std::vector<s16> ret(ret_size);

    int yn1 = state.yn1, yn2 = state.yn2;

    const std::size_t NUM_FRAMES =
        (sample_count + (SAMPLES_PER_FRAME - 1)) / SAMPLES_PER_FRAME; // Round up.
    for (std::size_t framei = 0; framei < NUM_FRAMES; framei++) {
        const int frame_header = data[framei * FRAME_LEN];
        const int scale = 1 << (frame_header & 0xF);
        const int idx = (frame_header >> 4) & 0x7;

        // Coefficients are fixed point with 11 bits fractional part.
        const int coef1 = coeff[idx * 2 + 0];
        const int coef2 = coeff[idx * 2 + 1];

        // Decodes an audio sample. One nibble produces one sample.
        const auto decode_sample = [&](const int nibble) -> s16 {
            const int xn = nibble * scale;
            // We first transform everything into 11 bit fixed point, perform the second order
            // digital filter, then transform back.
            // 0x400 == 0.5 in 11 bit fixed point.
            // Filter: y[n] = x[n] + 0.5 + c1 * y[n-1] + c2 * y[n-2]
            int val = ((xn << 11) + 0x400 + coef1 * yn1 + coef2 * yn2) >> 11;
            // Clamp to output range.
            val = std::clamp<s32>(val, -32768, 32767);
            // Advance output feedback.
            yn2 = yn1;
            yn1 = val;
            return static_cast<s16>(val);
        };

        std::size_t outputi = framei * SAMPLES_PER_FRAME;
        std::size_t datai = framei * FRAME_LEN + 1;
        for (std::size_t i = 0; i < SAMPLES_PER_FRAME && outputi < sample_count; i += 2) {
            const s16 sample1 = decode_sample(SIGNED_NIBBLES[data[datai] >> 4]);
            ret[outputi] = sample1;
            outputi++;

            const s16 sample2 = decode_sample(SIGNED_NIBBLES[data[datai] & 0xF]);
            ret[outputi] = sample2;
            outputi++;

            datai++;
        }
    }

    state.yn1 = static_cast<s16>(yn1);
    state.yn2 = static_cast<s16>(yn2);

    return ret;
}

} // namespace AudioCore::Codec
