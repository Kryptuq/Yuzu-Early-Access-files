/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/stdint.hpp>

namespace Dynarmic::Atomic {

inline void Or(volatile u32* ptr, u32 value) {
#ifdef _MSC_VER
    _InterlockedOr(reinterpret_cast<volatile long*>(ptr), value);
#else
    __atomic_or_fetch(ptr, value, __ATOMIC_SEQ_CST);
#endif
}

inline void And(volatile u32* ptr, u32 value) {
#ifdef _MSC_VER
    _InterlockedAnd(reinterpret_cast<volatile long*>(ptr), value);
#else
    __atomic_and_fetch(ptr, value, __ATOMIC_SEQ_CST);
#endif
}

}  // namespace Dynarmic::Atomic
