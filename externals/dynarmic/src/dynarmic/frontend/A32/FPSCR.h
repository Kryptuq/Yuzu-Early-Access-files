/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/fp/rounding_mode.h"

namespace Dynarmic::A32 {

/**
 * Representation of the Floating-Point Status and Control Register.
 */
class FPSCR final {
public:
    FPSCR() = default;
    FPSCR(const FPSCR&) = default;
    FPSCR(FPSCR&&) = default;
    explicit FPSCR(u32 data)
            : value{data & mask} {}

    FPSCR& operator=(const FPSCR&) = default;
    FPSCR& operator=(FPSCR&&) = default;
    FPSCR& operator=(u32 data) {
        value = data & mask;
        return *this;
    }

    /// Negative condition flag.
    bool N() const {
        return mcl::bit::get_bit<31>(value);
    }

    /// Zero condition flag.
    bool Z() const {
        return mcl::bit::get_bit<30>(value);
    }

    /// Carry condition flag.
    bool C() const {
        return mcl::bit::get_bit<29>(value);
    }

    /// Overflow condition flag.
    bool V() const {
        return mcl::bit::get_bit<28>(value);
    }

    /// Cumulative saturation flag.
    bool QC() const {
        return mcl::bit::get_bit<27>(value);
    }

    /// Alternate half-precision control flag.
    bool AHP() const {
        return mcl::bit::get_bit<26>(value);
    }

    /// Default NaN mode control bit.
    bool DN() const {
        return mcl::bit::get_bit<25>(value);
    }

    /// Flush-to-zero mode control bit.
    bool FTZ() const {
        return mcl::bit::get_bit<24>(value);
    }

    /// Rounding mode control field.
    FP::RoundingMode RMode() const {
        return static_cast<FP::RoundingMode>(mcl::bit::get_bits<22, 23>(value));
    }

    /// Indicates the stride of a vector.
    std::optional<size_t> Stride() const {
        switch (mcl::bit::get_bits<20, 21>(value)) {
        case 0b00:
            return 1;
        case 0b11:
            return 2;
        default:
            return std::nullopt;
        }
    }

    /// Indicates the length of a vector.
    size_t Len() const {
        return mcl::bit::get_bits<16, 18>(value) + 1;
    }

    /// Input denormal exception trap enable flag.
    bool IDE() const {
        return mcl::bit::get_bit<15>(value);
    }

    /// Inexact exception trap enable flag.
    bool IXE() const {
        return mcl::bit::get_bit<12>(value);
    }

    /// Underflow exception trap enable flag.
    bool UFE() const {
        return mcl::bit::get_bit<11>(value);
    }

    /// Overflow exception trap enable flag.
    bool OFE() const {
        return mcl::bit::get_bit<10>(value);
    }

    /// Division by zero exception trap enable flag.
    bool DZE() const {
        return mcl::bit::get_bit<9>(value);
    }

    /// Invalid operation exception trap enable flag.
    bool IOE() const {
        return mcl::bit::get_bit<8>(value);
    }

    /// Input denormal cumulative exception bit.
    bool IDC() const {
        return mcl::bit::get_bit<7>(value);
    }

    /// Inexact cumulative exception bit.
    bool IXC() const {
        return mcl::bit::get_bit<4>(value);
    }

    /// Underflow cumulative exception bit.
    bool UFC() const {
        return mcl::bit::get_bit<3>(value);
    }

    /// Overflow cumulative exception bit.
    bool OFC() const {
        return mcl::bit::get_bit<2>(value);
    }

    /// Division by zero cumulative exception bit.
    bool DZC() const {
        return mcl::bit::get_bit<1>(value);
    }

    /// Invalid operation cumulative exception bit.
    bool IOC() const {
        return mcl::bit::get_bit<0>(value);
    }

    /**
     * Whether or not the FPSCR indicates RunFast mode.
     *
     * RunFast mode is enabled when:
     *   - Flush-to-zero is enabled
     *   - Default NaNs are enabled.
     *   - All exception enable bits are cleared.
     */
    bool InRunFastMode() const {
        constexpr u32 runfast_mask = 0x03001F00;
        constexpr u32 expected = 0x03000000;

        return (value & runfast_mask) == expected;
    }

    /// Gets the underlying raw value within the FPSCR.
    u32 Value() const {
        return value;
    }

private:
    // Bits 5-6, 13-14, and 19 are reserved.
    static constexpr u32 mask = 0xFFF79F9F;
    u32 value = 0;
};

inline bool operator==(FPSCR lhs, FPSCR rhs) {
    return lhs.Value() == rhs.Value();
}

inline bool operator!=(FPSCR lhs, FPSCR rhs) {
    return !operator==(lhs, rhs);
}

}  // namespace Dynarmic::A32
