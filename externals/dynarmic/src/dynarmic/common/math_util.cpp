/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/math_util.h"

#include <array>

namespace Dynarmic::Common {

u8 RecipEstimate(u64 a) {
    using LUT = std::array<u8, 256>;
    static constexpr u64 lut_offset = 256;

    static const LUT lut = [] {
        LUT result{};
        for (u64 i = 0; i < result.size(); i++) {
            u64 a = i + lut_offset;

            a = a * 2 + 1;
            u64 b = (1u << 19) / a;
            result[i] = static_cast<u8>((b + 1) / 2);
        }
        return result;
    }();

    return lut[a - lut_offset];
}

/// Input is a u0.9 fixed point number. Only values in [0.25, 1.0) are valid.
/// Output is a u0.8 fixed point number, with an implied 1 prefixed.
/// i.e.: The output is a value in [1.0, 2.0).
u8 RecipSqrtEstimate(u64 a) {
    using LUT = std::array<u8, 512>;

    static const LUT lut = [] {
        LUT result{};
        for (u64 i = 128; i < result.size(); i++) {
            u64 a = i;

            // Convert to u.10 (with 8 significant bits), force to odd
            if (a < 256) {
                // [0.25, 0.5)
                a = a * 2 + 1;
            } else {
                // [0.5, 1.0)
                a = (a | 1) * 2;
            }

            // Calculate largest b which for which b < 1.0 / sqrt(a).
            // Start from b = 1.0 (in u.9) since b cannot be smaller.
            u64 b = 512;
            // u.10 * u.9 * u.9 -> u.28
            while (a * (b + 1) * (b + 1) < (1u << 28)) {
                b++;
            }

            // Round to nearest u0.8 (with implied set integer bit).
            result[i] = static_cast<u8>((b + 1) / 2);
        }
        return result;
    }();

    return lut[a & 0x1FF];
}

}  // namespace Dynarmic::Common
