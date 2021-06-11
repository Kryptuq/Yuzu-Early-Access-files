/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>

#include "dynarmic/common/common_types.h"

namespace Dynarmic::Backend::X64 {

constexpr size_t SpillCount = 64;

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4324)  // Structure was padded due to alignment specifier
#endif

struct alignas(16) StackLayout {
    s64 cycles_remaining;
    s64 cycles_to_run;

    std::array<std::array<u64, 2>, SpillCount> spill;

    u32 save_host_MXCSR;

    bool check_bit;
    bool halt_requested;
};

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

static_assert(sizeof(StackLayout) % 16 == 0);

}  // namespace Dynarmic::Backend::X64
