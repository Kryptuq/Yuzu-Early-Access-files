/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstdint>

namespace Dynarmic {

enum class OptimizationFlag : std::uint32_t {
    /// This optimization avoids dispatcher lookups by allowing emitted basic blocks to jump
    /// directly to other basic blocks if the destination PC is predictable at JIT-time.
    /// This is a safe optimization.
    BlockLinking = 0x00000001,
    /// This optimization avoids dispatcher lookups by emulating a return stack buffer. This
    /// allows for function returns and syscall returns to be predicted at runtime.
    /// This is a safe optimization.
    ReturnStackBuffer = 0x00000002,
    /// This optimization enables a two-tiered dispatch system.
    /// A fast dispatcher (written in assembly) first does a look-up in a small MRU cache.
    /// If this fails, it falls back to the usual slower dispatcher.
    /// This is a safe optimization.
    FastDispatch = 0x00000004,
    /// This is an IR optimization. This optimization eliminates unnecessary emulated CPU state
    /// context lookups.
    /// This is a safe optimization.
    GetSetElimination = 0x00000008,
    /// This is an IR optimization. This optimization does constant propagation.
    /// This is a safe optimization.
    ConstProp = 0x00000010,
    /// This is enables miscellaneous safe IR optimizations.
    MiscIROpt = 0x00000020,

    /// This is an UNSAFE optimization that reduces accuracy of fused multiply-add operations.
    /// This unfuses fused instructions to improve performance on host CPUs without FMA support.
    Unsafe_UnfuseFMA = 0x00010000,
    /// This is an UNSAFE optimization that reduces accuracy of certain floating-point instructions.
    /// This allows results of FRECPE and FRSQRTE to have **less** error than spec allows.
    Unsafe_ReducedErrorFP = 0x00020000,
    /// This is an UNSAFE optimization that causes floating-point instructions to not produce correct NaNs.
    /// This may also result in inaccurate results when instructions are given certain special values.
    Unsafe_InaccurateNaN = 0x00040000,
    /// This is an UNSAFE optimization that causes ASIMD floating-point instructions to be run with incorrect
    /// rounding modes. This may result in inaccurate results with all floating-point ASIMD instructions.
    Unsafe_IgnoreStandardFPCRValue = 0x00080000,
};

constexpr OptimizationFlag no_optimizations = static_cast<OptimizationFlag>(0);
constexpr OptimizationFlag all_safe_optimizations = static_cast<OptimizationFlag>(0x0000FFFF);

constexpr OptimizationFlag operator~(OptimizationFlag f) {
    return static_cast<OptimizationFlag>(~static_cast<std::uint32_t>(f));
}

constexpr OptimizationFlag operator|(OptimizationFlag f1, OptimizationFlag f2) {
    return static_cast<OptimizationFlag>(static_cast<std::uint32_t>(f1) | static_cast<std::uint32_t>(f2));
}

constexpr OptimizationFlag operator&(OptimizationFlag f1, OptimizationFlag f2) {
    return static_cast<OptimizationFlag>(static_cast<std::uint32_t>(f1) & static_cast<std::uint32_t>(f2));
}

constexpr OptimizationFlag operator|=(OptimizationFlag& result, OptimizationFlag f) {
    return result = (result | f);
}

constexpr OptimizationFlag operator&=(OptimizationFlag& result, OptimizationFlag f) {
    return result = (result & f);
}

constexpr bool operator!(OptimizationFlag f) {
    return f == no_optimizations;
}

}  // namespace Dynarmic
