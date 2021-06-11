/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/lift_value.h>

namespace mp {

/// Metafunction that returns the number of arguments it has
template<typename... Ts>
using argument_count = lift_value<sizeof...(Ts)>;

/// Metafunction that returns the number of arguments it has
template<typename... Ts>
constexpr auto argument_count_v = sizeof...(Ts);

} // namespace mp
