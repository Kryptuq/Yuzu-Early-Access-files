/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

#include <mcl/assert.hpp>
#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/fp/rounding_mode.h"

namespace Dynarmic::FP {

/**
 * Representation of the Floating-Point Control Register.
 */
class FPCR final {
public:
    FPCR() = default;
    FPCR(const FPCR&) = default;
    FPCR(FPCR&&) = default;
    explicit FPCR(u32 data)
            : value{data & mask} {}

    FPCR& operator=(const FPCR&) = default;
    FPCR& operator=(FPCR&&) = default;
    FPCR& operator=(u32 data) {
        value = data & mask;
        return *this;
    }

    /// Get alternate half-precision control flag.
    bool AHP() const {
        return mcl::bit::get_bit<26>(value);
    }

    /// Set alternate half-precision control flag.
    void AHP(bool ahp) {
        value = mcl::bit::set_bit<26>(value, ahp);
    }

    /// Get default NaN mode control bit.
    bool DN() const {
        return mcl::bit::get_bit<25>(value);
    }

    /// Set default NaN mode control bit.
    void DN(bool dn) {
        value = mcl::bit::set_bit<25>(value, dn);
    }

    /// Get flush-to-zero mode control bit.
    bool FZ() const {
        return mcl::bit::get_bit<24>(value);
    }

    /// Set flush-to-zero mode control bit.
    void FZ(bool fz) {
        value = mcl::bit::set_bit<24>(value, fz);
    }

    /// Get rounding mode control field.
    FP::RoundingMode RMode() const {
        return static_cast<FP::RoundingMode>(mcl::bit::get_bits<22, 23>(value));
    }

    /// Set rounding mode control field.
    void RMode(FP::RoundingMode rounding_mode) {
        ASSERT_MSG(static_cast<u32>(rounding_mode) <= 0b11, "FPCR: Invalid rounding mode");
        value = mcl::bit::set_bits<22, 23>(value, static_cast<u32>(rounding_mode));
    }

    /// Get the stride of a vector when executing AArch32 VFP instructions.
    /// This field has no function in AArch64 state.
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

    /// Set the stride of a vector when executing AArch32 VFP instructions.
    /// This field has no function in AArch64 state.
    void Stride(size_t stride) {
        ASSERT_MSG(stride >= 1 && stride <= 2, "FPCR: Invalid stride");
        value = mcl::bit::set_bits<20, 21>(value, stride == 1 ? 0b00u : 0b11u);
    }

    /// Get flush-to-zero (half-precision specific) mode control bit.
    bool FZ16() const {
        return mcl::bit::get_bit<19>(value);
    }

    /// Set flush-to-zero (half-precision specific) mode control bit.
    void FZ16(bool fz16) {
        value = mcl::bit::set_bit<19>(value, fz16);
    }

    /// Gets the length of a vector when executing AArch32 VFP instructions.
    /// This field has no function in AArch64 state.
    size_t Len() const {
        return mcl::bit::get_bits<16, 18>(value) + 1;
    }

    /// Sets the length of a vector when executing AArch32 VFP instructions.
    /// This field has no function in AArch64 state.
    void Len(size_t len) {
        ASSERT_MSG(len >= 1 && len <= 8, "FPCR: Invalid len");
        value = mcl::bit::set_bits<16, 18>(value, static_cast<u32>(len - 1));
    }

    /// Get input denormal exception trap enable flag.
    bool IDE() const {
        return mcl::bit::get_bit<15>(value);
    }

    /// Set input denormal exception trap enable flag.
    void IDE(bool ide) {
        value = mcl::bit::set_bit<15>(value, ide);
    }

    /// Get inexact exception trap enable flag.
    bool IXE() const {
        return mcl::bit::get_bit<12>(value);
    }

    /// Set inexact exception trap enable flag.
    void IXE(bool ixe) {
        value = mcl::bit::set_bit<12>(value, ixe);
    }

    /// Get underflow exception trap enable flag.
    bool UFE() const {
        return mcl::bit::get_bit<11>(value);
    }

    /// Set underflow exception trap enable flag.
    void UFE(bool ufe) {
        value = mcl::bit::set_bit<11>(value, ufe);
    }

    /// Get overflow exception trap enable flag.
    bool OFE() const {
        return mcl::bit::get_bit<10>(value);
    }

    /// Set overflow exception trap enable flag.
    void OFE(bool ofe) {
        value = mcl::bit::set_bit<10>(value, ofe);
    }

    /// Get division by zero exception trap enable flag.
    bool DZE() const {
        return mcl::bit::get_bit<9>(value);
    }

    /// Set division by zero exception trap enable flag.
    void DZE(bool dze) {
        value = mcl::bit::set_bit<9>(value, dze);
    }

    /// Get invalid operation exception trap enable flag.
    bool IOE() const {
        return mcl::bit::get_bit<8>(value);
    }

    /// Set invalid operation exception trap enable flag.
    void IOE(bool ioe) {
        value = mcl::bit::set_bit<8>(value, ioe);
    }

    /// Gets the underlying raw value within the FPCR.
    u32 Value() const {
        return value;
    }

    /// Gets the StandardFPSCRValue (A32 ASIMD).
    FPCR ASIMDStandardValue() const {
        FPCR stdvalue;
        stdvalue.AHP(AHP());
        stdvalue.FZ16(FZ16());
        stdvalue.FZ(true);
        stdvalue.DN(true);
        return stdvalue;
    }

private:
    // Bits 0-7, 13-14, and 27-31 are reserved.
    static constexpr u32 mask = 0x07FF9F00;
    u32 value = 0;
};

inline bool operator==(FPCR lhs, FPCR rhs) {
    return lhs.Value() == rhs.Value();
}

inline bool operator!=(FPCR lhs, FPCR rhs) {
    return !operator==(lhs, rhs);
}

}  // namespace Dynarmic::FP
