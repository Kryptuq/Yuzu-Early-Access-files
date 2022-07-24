// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/lift_value.hpp"

namespace mcl::mp {

/// Product of metavalues Vs
template<class... Vs>
using product = lift_value<(Vs::value * ...)>;

/// Product of metavalues Vs
template<class... Vs>
constexpr auto product_v = (Vs::value * ...);

}  // namespace mcl::mp
