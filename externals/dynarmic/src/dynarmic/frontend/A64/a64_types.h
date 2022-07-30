/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <string>

#include <fmt/format.h>
#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/ir/cond.h"

namespace Dynarmic::A64 {

using Cond = IR::Cond;

enum class Reg {
    R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    R16,
    R17,
    R18,
    R19,
    R20,
    R21,
    R22,
    R23,
    R24,
    R25,
    R26,
    R27,
    R28,
    R29,
    R30,
    R31,
    LR = R30,
    SP = R31,
    ZR = R31,
};

enum class Vec {
    V0,
    V1,
    V2,
    V3,
    V4,
    V5,
    V6,
    V7,
    V8,
    V9,
    V10,
    V11,
    V12,
    V13,
    V14,
    V15,
    V16,
    V17,
    V18,
    V19,
    V20,
    V21,
    V22,
    V23,
    V24,
    V25,
    V26,
    V27,
    V28,
    V29,
    V30,
    V31,
};

enum class ShiftType {
    LSL,
    LSR,
    ASR,
    ROR,
};

const char* CondToString(Cond cond);
std::string RegToString(Reg reg);
std::string VecToString(Vec vec);

constexpr size_t RegNumber(Reg reg) {
    return static_cast<size_t>(reg);
}

constexpr size_t VecNumber(Vec vec) {
    return static_cast<size_t>(vec);
}

inline Reg operator+(Reg reg, size_t number) {
    const size_t new_reg = RegNumber(reg) + number;
    ASSERT(new_reg <= 31);

    return static_cast<Reg>(new_reg);
}

inline Vec operator+(Vec vec, size_t number) {
    const size_t new_vec = VecNumber(vec) + number;
    ASSERT(new_vec <= 31);

    return static_cast<Vec>(new_vec);
}

}  // namespace Dynarmic::A64

template<>
struct fmt::formatter<Dynarmic::A64::Reg> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(Dynarmic::A64::Reg reg, FormatContext& ctx) const {
        return formatter<std::string>::format(Dynarmic::A64::RegToString(reg), ctx);
    }
};

template<>
struct fmt::formatter<Dynarmic::A64::Vec> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(Dynarmic::A64::Vec vec, FormatContext& ctx) const {
        return formatter<std::string>::format(Dynarmic::A64::VecToString(vec), ctx);
    }
};
