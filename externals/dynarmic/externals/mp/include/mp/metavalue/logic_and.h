/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/value.h>

namespace mp {

/// Logical conjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
using logic_and = bool_value<(true && ... && Vs::value)>;

/// Logical conjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
constexpr bool logic_and_v = (true && ... && Vs::value);

} // namespace mp
