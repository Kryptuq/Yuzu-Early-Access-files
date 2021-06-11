/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/lift_value.h>

namespace mp {

/// Sum of metavalues Vs
template<class... Vs>
using sum = lift_value<(Vs::value + ...)>;

/// Sum of metavalues Vs
template<class... Vs>
constexpr auto sum_v = (Vs::value + ...);

} // namespace mp
