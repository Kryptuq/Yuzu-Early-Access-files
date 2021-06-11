/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/lift_value.h>

namespace mp {

/// Product of metavalues Vs
template<class... Vs>
using product = lift_value<(Vs::value * ...)>;

/// Product of metavalues Vs
template<class... Vs>
constexpr auto product_v = (Vs::value * ...);

} // namespace mp
