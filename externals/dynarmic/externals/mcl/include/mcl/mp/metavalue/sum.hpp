// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/lift_value.hpp"

namespace mcl::mp {

/// Sum of metavalues Vs
template<class... Vs>
using sum = lift_value<(Vs::value + ...)>;

/// Sum of metavalues Vs
template<class... Vs>
constexpr auto sum_v = (Vs::value + ...);

}  // namespace mcl::mp
