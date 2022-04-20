// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/concepts/is_any_of.hpp"
#include "mcl/stdint.hpp"

namespace mcl {

/// Integral upon which bit operations can be safely performed.
template<typename T>
concept BitIntegral = IsAnyOf<T, u8, u16, u32, u64, uptr, size_t>;

}  // namespace mcl
