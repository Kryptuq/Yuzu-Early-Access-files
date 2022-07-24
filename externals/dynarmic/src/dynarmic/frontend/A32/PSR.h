/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/frontend/A32/ITState.h"

namespace Dynarmic::A32 {

/**
 * Program Status Register
 *
 * | Bit(s)  | Description                                   |
 * |:-------:|:----------------------------------------------|
 * | N       | Negative                                      |
 * | Z       | Zero                                          |
 * | C       | Carry                                         |
 * | V       | Overflow                                      |
 * | Q       | Sticky overflow for DSP-oriented instructions |
 * | IT[1:0] | Lower two bits of the If-Then execution state |
 * | J       | Jazelle bit                                   |
 * | GE      | Greater-than or Equal                         |
 * | IT[7:2] | Upper six bits of the If-Then execution state |
 * | E       | Endian (0 is little endian, 1 is big endian)  |
 * | A       | Imprecise data abort (disables them when set) |
 * | I       | IRQ interrupts (disabled when set)            |
 * | F       | FIQ interrupts (disabled when set)            |
 * | T       | Thumb bit                                     |
 * | M       | Current processor mode                        |
 */
class PSR final {
public:
    /// Valid processor modes that may be indicated.
    enum class Mode : u32 {
        User = 0b10000,
        FIQ = 0b10001,
        IRQ = 0b10010,
        Supervisor = 0b10011,
        Monitor = 0b10110,
        Abort = 0b10111,
        Hypervisor = 0b11010,
        Undefined = 0b11011,
        System = 0b11111
    };

    /// Instruction sets that may be signified through a PSR.
    enum class InstructionSet {
        ARM,
        Jazelle,
        Thumb,
        ThumbEE
    };

    PSR() = default;
    explicit PSR(u32 data)
            : value{data & mask} {}

    PSR& operator=(u32 data) {
        value = data & mask;
        return *this;
    }

    bool N() const {
        return mcl::bit::get_bit<31>(value);
    }
    void N(bool set) {
        value = mcl::bit::set_bit<31>(value, set);
    }

    bool Z() const {
        return mcl::bit::get_bit<30>(value);
    }
    void Z(bool set) {
        value = mcl::bit::set_bit<30>(value, set);
    }

    bool C() const {
        return mcl::bit::get_bit<29>(value);
    }
    void C(bool set) {
        value = mcl::bit::set_bit<29>(value, set);
    }

    bool V() const {
        return mcl::bit::get_bit<28>(value);
    }
    void V(bool set) {
        value = mcl::bit::set_bit<28>(value, set);
    }

    bool Q() const {
        return mcl::bit::get_bit<27>(value);
    }
    void Q(bool set) {
        value = mcl::bit::set_bit<27>(value, set);
    }

    bool J() const {
        return mcl::bit::get_bit<24>(value);
    }
    void J(bool set) {
        value = mcl::bit::set_bit<24>(value, set);
    }

    u32 GE() const {
        return mcl::bit::get_bits<16, 19>(value);
    }
    void GE(u32 data) {
        value = mcl::bit::set_bits<16, 19>(value, data);
    }

    ITState IT() const {
        return ITState{static_cast<u8>((value & 0x6000000) >> 25 | (value & 0xFC00) >> 8)};
    }
    void IT(ITState it_state) {
        const u32 data = it_state.Value();
        value = (value & ~0x000FC00) | (data & 0b11111100) << 8;
        value = (value & ~0x6000000) | (data & 0b00000011) << 25;
    }

    bool E() const {
        return mcl::bit::get_bit<9>(value);
    }
    void E(bool set) {
        value = mcl::bit::set_bit<9>(value, set);
    }

    bool A() const {
        return mcl::bit::get_bit<8>(value);
    }
    void A(bool set) {
        value = mcl::bit::set_bit<8>(value, set);
    }

    bool I() const {
        return mcl::bit::get_bit<7>(value);
    }
    void I(bool set) {
        value = mcl::bit::set_bit<7>(value, set);
    }

    bool F() const {
        return mcl::bit::get_bit<6>(value);
    }
    void F(bool set) {
        value = mcl::bit::set_bit<6>(value, set);
    }

    bool T() const {
        return mcl::bit::get_bit<5>(value);
    }
    void T(bool set) {
        value = mcl::bit::set_bit<5>(value, set);
    }

    Mode M() const {
        return static_cast<Mode>(mcl::bit::get_bits<0, 4>(value));
    }
    void M(Mode mode) {
        value = mcl::bit::set_bits<0, 4>(value, static_cast<u32>(mode));
    }

    u32 Value() const {
        return value;
    }

    InstructionSet CurrentInstructionSet() const {
        const bool j_bit = J();
        const bool t_bit = T();

        if (j_bit && t_bit)
            return InstructionSet::ThumbEE;

        if (t_bit)
            return InstructionSet::Thumb;

        if (j_bit)
            return InstructionSet::Jazelle;

        return InstructionSet::ARM;
    }

    void CurrentInstructionSet(InstructionSet instruction_set) {
        switch (instruction_set) {
        case InstructionSet::ARM:
            T(false);
            J(false);
            break;
        case InstructionSet::Jazelle:
            T(false);
            J(true);
            break;
        case InstructionSet::Thumb:
            T(true);
            J(false);
            break;
        case InstructionSet::ThumbEE:
            T(true);
            J(true);
            break;
        }
    }

private:
    // Bits 20-23 are reserved and should be zero.
    static constexpr u32 mask = 0xFF0FFFFF;

    u32 value = 0;
};

inline bool operator==(PSR lhs, PSR rhs) {
    return lhs.Value() == rhs.Value();
}

inline bool operator!=(PSR lhs, PSR rhs) {
    return !operator==(lhs, rhs);
}

}  // namespace Dynarmic::A32
