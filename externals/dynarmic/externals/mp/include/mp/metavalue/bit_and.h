/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/lift_value.h>

namespace mp {

/// Bitwise and of metavalues Vs
template<class... Vs>
using bit_and = lift_value<(Vs::value & ...)>;

/// Bitwise and of metavalues Vs
template<class... Vs>
constexpr auto bit_and_v = (Vs::value & ...);

} // namespace mp
