/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/value.h>

namespace mp {

/// Logical disjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
using logic_or = bool_value<(false || ... || Vs::value)>;

/// Logical disjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
constexpr bool logic_or_v = (false || ... || Vs::value);

} // namespace mp
