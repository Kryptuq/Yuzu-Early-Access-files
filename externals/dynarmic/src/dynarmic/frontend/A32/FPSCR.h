/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
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
        return Common::Bit<31>(value);
    }

    /// Zero condition flag.
    bool Z() const {
        return Common::Bit<30>(value);
    }

    /// Carry condition flag.
    bool C() const {
        return Common::Bit<29>(value);
    }

    /// Overflow condition flag.
    bool V() const {
        return Common::Bit<28>(value);
    }

    /// Cumulative saturation flag.
    bool QC() const {
        return Common::Bit<27>(value);
    }

    /// Alternate half-precision control flag.
    bool AHP() const {
        return Common::Bit<26>(value);
    }

    /// Default NaN mode control bit.
    bool DN() const {
        return Common::Bit<25>(value);
    }

    /// Flush-to-zero mode control bit.
    bool FTZ() const {
        return Common::Bit<24>(value);
    }

    /// Rounding mode control field.
    FP::RoundingMode RMode() const {
        return static_cast<FP::RoundingMode>(Common::Bits<22, 23>(value));
    }

    /// Indicates the stride of a vector.
    std::optional<size_t> Stride() const {
        switch (Common::Bits<20, 21>(value)) {
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
        return Common::Bits<16, 18>(value) + 1;
    }

    /// Input denormal exception trap enable flag.
    bool IDE() const {
        return Common::Bit<15>(value);
    }

    /// Inexact exception trap enable flag.
    bool IXE() const {
        return Common::Bit<12>(value);
    }

    /// Underflow exception trap enable flag.
    bool UFE() const {
        return Common::Bit<11>(value);
    }

    /// Overflow exception trap enable flag.
    bool OFE() const {
        return Common::Bit<10>(value);
    }

    /// Division by zero exception trap enable flag.
    bool DZE() const {
        return Common::Bit<9>(value);
    }

    /// Invalid operation exception trap enable flag.
    bool IOE() const {
        return Common::Bit<8>(value);
    }

    /// Input denormal cumulative exception bit.
    bool IDC() const {
        return Common::Bit<7>(value);
    }

    /// Inexact cumulative exception bit.
    bool IXC() const {
        return Common::Bit<4>(value);
    }

    /// Underflow cumulative exception bit.
    bool UFC() const {
        return Common::Bit<3>(value);
    }

    /// Overflow cumulative exception bit.
    bool OFC() const {
        return Common::Bit<2>(value);
    }

    /// Division by zero cumulative exception bit.
    bool DZC() const {
        return Common::Bit<1>(value);
    }

    /// Invalid operation cumulative exception bit.
    bool IOC() const {
        return Common::Bit<0>(value);
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
