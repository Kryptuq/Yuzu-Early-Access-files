/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace Dynarmic::FP {

/// Ordering of first four values is important as they correspond to bits in FPCR.
enum class RoundingMode {
    /// Round to nearest floating point. If there is a tie, round to nearest even digit in required position.
    ToNearest_TieEven,
    /// Round up towards positive infinity.
    TowardsPlusInfinity,
    /// Round downwards towards negative infinity.
    TowardsMinusInfinity,
    /// Truncate towards zero.
    TowardsZero,
    /// Round to nearest floating point. If there is a tie, round away from zero.
    ToNearest_TieAwayFromZero,
    /// Von Neumann rounding (as modified by Brent). Also known as sticky rounding.
    /// Set the least significant bit to 1 if the result is not exact.
    ToOdd,
};

}  // namespace Dynarmic::FP
