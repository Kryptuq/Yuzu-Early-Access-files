/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/lift_value.h>

namespace mp {

/// Bitwise or of metavalues Vs
template<class... Vs>
using bit_or = lift_value<(Vs::value | ...)>;

/// Bitwise or of metavalues Vs
template<class... Vs>
constexpr auto bit_or_v = (Vs::value | ...);

} // namespace mp
