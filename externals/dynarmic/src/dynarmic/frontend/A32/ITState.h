/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/ir/cond.h"

namespace Dynarmic::A32 {

class ITState final {
public:
    ITState() = default;
    explicit ITState(u8 data)
            : value(data) {}

    ITState& operator=(u8 data) {
        value = data;
        return *this;
    }

    IR::Cond Cond() const {
        if (value == 0b00000000) {
            return IR::Cond::AL;
        }
        return static_cast<IR::Cond>(mcl::bit::get_bits<4, 7>(value));
    }

    bool IsInITBlock() const {
        return mcl::bit::get_bits<0, 3>(value) != 0b0000;
    }

    bool IsLastInITBlock() const {
        return mcl::bit::get_bits<0, 3>(value) == 0b1000;
    }

    ITState Advance() const {
        if (mcl::bit::get_bits<0, 2>(value) == 0b000) {
            return ITState{0b00000000};
        }
        return ITState{mcl::bit::set_bits<0, 4>(value, static_cast<u8>(mcl::bit::get_bits<0, 4>(value) << 1))};
    }

    u8 Value() const {
        return value;
    }

private:
    u8 value = 0;
};

inline bool operator==(ITState lhs, ITState rhs) {
    return lhs.Value() == rhs.Value();
}

inline bool operator!=(ITState lhs, ITState rhs) {
    return !operator==(lhs, rhs);
}

}  // namespace Dynarmic::A32
