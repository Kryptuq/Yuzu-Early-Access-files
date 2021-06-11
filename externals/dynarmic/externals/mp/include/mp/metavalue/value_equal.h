/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <type_traits>

namespace mp {

/// Do two metavalues contain the same value?
template<class V1, class V2>
using value_equal = std::bool_constant<V1::value == V2::value>;

} // namespace mp
