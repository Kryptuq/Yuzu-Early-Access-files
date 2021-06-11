/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/value.h>

namespace mp {

/// Logical negation of metavalue V.
template<class V>
using logic_not = bool_value<!bool(V::value)>;

/// Logical negation of metavalue V.
template<class V>
constexpr bool logic_not_v = !bool(V::value);

} // namespace mp
