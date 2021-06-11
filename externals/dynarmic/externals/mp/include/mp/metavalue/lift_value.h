/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <type_traits>

namespace mp {

/// Lifts a value into a type (a metavalue)
template<auto V>
using lift_value = std::integral_constant<decltype(V), V>;

} // namespace mp
