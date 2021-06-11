/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <type_traits>
#include <mp/metavalue/value.h>

namespace mp {

/// Conditionally select between types T and F based on boolean metavalue V
template<class V, class T, class F>
using logic_if = std::conditional_t<bool(V::value), T, F>;

/// Conditionally select between metavalues T and F based on boolean metavalue V
template<class V, class TV, class FV>
constexpr auto logic_if_v = logic_if<V, TV, FV>::value;

} // namespace mp
