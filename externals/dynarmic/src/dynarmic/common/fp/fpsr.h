/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"

namespace Dynarmic::FP {

/**
 * Representation of the Floating-Point Status Register.
 */
class FPSR final {
public:
    FPSR() = default;
    FPSR(const FPSR&) = default;
    FPSR(FPSR&&) = default;
    explicit FPSR(u32 data)
            : value{data & mask} {}

    FPSR& operator=(const FPSR&) = default;
    FPSR& operator=(FPSR&&) = default;
    FPSR& operator=(u32 data) {
        value = data & mask;
        return *this;
    }

    /// Get negative condition flag
    bool N() const {
        return Common::Bit<31>(value);
    }

    /// Set negative condition flag
    void N(bool N_) {
        value = Common::ModifyBit<31>(value, N_);
    }

    /// Get zero condition flag
    bool Z() const {
        return Common::Bit<30>(value);
    }

    /// Set zero condition flag
    void Z(bool Z_) {
        value = Common::ModifyBit<30>(value, Z_);
    }

    /// Get carry condition flag
    bool C() const {
        return Common::Bit<29>(value);
    }

    /// Set carry condition flag
    void C(bool C_) {
        value = Common::ModifyBit<29>(value, C_);
    }

    /// Get overflow condition flag
    bool V() const {
        return Common::Bit<28>(value);
    }

    /// Set overflow condition flag
    void V(bool V_) {
        value = Common::ModifyBit<28>(value, V_);
    }

    /// Get cumulative saturation bit
    bool QC() const {
        return Common::Bit<27>(value);
    }

    /// Set cumulative saturation bit
    void QC(bool QC_) {
        value = Common::ModifyBit<27>(value, QC_);
    }

    /// Get input denormal floating-point exception bit
    bool IDC() const {
        return Common::Bit<7>(value);
    }

    /// Set input denormal floating-point exception bit
    void IDC(bool IDC_) {
        value = Common::ModifyBit<7>(value, IDC_);
    }

    /// Get inexact cumulative floating-point exception bit
    bool IXC() const {
        return Common::Bit<4>(value);
    }

    /// Set inexact cumulative floating-point exception bit
    void IXC(bool IXC_) {
        value = Common::ModifyBit<4>(value, IXC_);
    }

    /// Get underflow cumulative floating-point exception bit
    bool UFC() const {
        return Common::Bit<3>(value);
    }

    /// Set underflow cumulative floating-point exception bit
    void UFC(bool UFC_) {
        value = Common::ModifyBit<3>(value, UFC_);
    }

    /// Get overflow cumulative floating-point exception bit
    bool OFC() const {
        return Common::Bit<2>(value);
    }

    /// Set overflow cumulative floating-point exception bit
    void OFC(bool OFC_) {
        value = Common::ModifyBit<2>(value, OFC_);
    }

    /// Get divide by zero cumulative floating-point exception bit
    bool DZC() const {
        return Common::Bit<1>(value);
    }

    /// Set divide by zero cumulative floating-point exception bit
    void DZC(bool DZC_) {
        value = Common::ModifyBit<1>(value, DZC_);
    }

    /// Get invalid operation cumulative floating-point exception bit
    bool IOC() const {
        return Common::Bit<0>(value);
    }

    /// Set invalid operation cumulative floating-point exception bit
    void IOC(bool IOC_) {
        value = Common::ModifyBit<0>(value, IOC_);
    }

    /// Gets the underlying raw value within the FPSR.
    u32 Value() const {
        return value;
    }

private:
    // Bits 5-6 and 8-26 are reserved.
    static constexpr u32 mask = 0xF800009F;
    u32 value = 0;
};

inline bool operator==(FPSR lhs, FPSR rhs) {
    return lhs.Value() == rhs.Value();
}

inline bool operator!=(FPSR lhs, FPSR rhs) {
    return !operator==(lhs, rhs);
}

}  // namespace Dynarmic::FP
