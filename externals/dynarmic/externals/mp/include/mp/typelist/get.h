/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstddef>
#include <tuple>

#include <mp/metafunction/apply.h>

namespace mp {

/// Get element I from list L
template<std::size_t I, class L>
using get = std::tuple_element_t<I, apply<std::tuple, L>>;

} // namespace mp
