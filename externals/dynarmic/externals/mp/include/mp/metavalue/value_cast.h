/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <type_traits>

namespace mp {

/// Casts a metavalue from one type to another
template<class T, class V>
using value_cast = std::integral_constant<T, static_cast<T>(V::value)>;

} // namespace mp
