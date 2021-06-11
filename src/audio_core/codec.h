// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>

#include "common/common_types.h"

namespace AudioCore::Codec {

enum class PcmFormat : u32 {
    Invalid = 0,
    Int8 = 1,
    Int16 = 2,
    Int24 = 3,
    Int32 = 4,
    PcmFloat = 5,
    Adpcm = 6,
};

/// See: Codec::DecodeADPCM
struct ADPCMState {
    // Two historical samples from previous processed buffer,
    // required for ADPCM decoding
    s16 yn1; ///< y[n-1]
    s16 yn2; ///< y[n-2]
};

using ADPCM_Coeff = std::array<s16, 16>;

/**
 * @param data Pointer to buffer that contains ADPCM data to decode
 * @param size Size of buffer in bytes
 * @param coeff ADPCM coefficients
 * @param state ADPCM state, this is updated with new state
 * @return Decoded stereo signed PCM16 data, sample_count in length
 */
std::vector<s16> DecodeADPCM(const u8* data, std::size_t size, const ADPCM_Coeff& coeff,
                             ADPCMState& state);

}; // namespace AudioCore::Codec
