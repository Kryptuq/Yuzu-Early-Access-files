// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/lift_value.hpp"

namespace mcl::mp {

/// Bitwise or of metavalues Vs
template<class... Vs>
using bit_or = lift_value<(Vs::value | ...)>;

/// Bitwise or of metavalues Vs
template<class... Vs>
constexpr auto bit_or_v = (Vs::value | ...);

}  // namespace mcl::mp
