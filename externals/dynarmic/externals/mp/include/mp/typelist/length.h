/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metafunction/apply.h>
#include <mp/misc/argument_count.h>

namespace mp {

/// Length of list L
template<class L>
using length = apply<argument_count, L>;

/// Length of list L
template<class L>
constexpr auto length_v = length<L>::value;

} // namespace mp
