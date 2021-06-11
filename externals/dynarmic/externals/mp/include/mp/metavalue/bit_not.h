/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/lift_value.h>

namespace mp {

/// Bitwise not of metavalue V
template<class V>
using bit_not = lift_value<~V::value>;

/// Bitwise not of metavalue V
template<class V>
constexpr auto bit_not_v = ~V::value;

} // namespace mp
