/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <string>

#include <fmt/format.h>
#include <mcl/stdint.hpp>

namespace Dynarmic::IR {

/**
 * The intermediate representation is typed. These are the used by our IR.
 */
enum class Type {
    Void = 0,
    A32Reg = 1 << 0,
    A32ExtReg = 1 << 1,
    A64Reg = 1 << 2,
    A64Vec = 1 << 3,
    Opaque = 1 << 4,
    U1 = 1 << 5,
    U8 = 1 << 6,
    U16 = 1 << 7,
    U32 = 1 << 8,
    U64 = 1 << 9,
    U128 = 1 << 10,
    CoprocInfo = 1 << 11,
    NZCVFlags = 1 << 12,
    Cond = 1 << 13,
    Table = 1 << 14,
    AccType = 1 << 15,
};

constexpr Type operator|(Type a, Type b) {
    return static_cast<Type>(static_cast<size_t>(a) | static_cast<size_t>(b));
}

constexpr Type operator&(Type a, Type b) {
    return static_cast<Type>(static_cast<size_t>(a) & static_cast<size_t>(b));
}

/// Get the name of a type.
std::string GetNameOf(Type type);

/// @returns true if t1 and t2 are compatible types
bool AreTypesCompatible(Type t1, Type t2);

}  // namespace Dynarmic::IR

template<>
struct fmt::formatter<Dynarmic::IR::Type> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(Dynarmic::IR::Type type, FormatContext& ctx) const {
        return formatter<std::string>::format(Dynarmic::IR::GetNameOf(type), ctx);
    }
};
