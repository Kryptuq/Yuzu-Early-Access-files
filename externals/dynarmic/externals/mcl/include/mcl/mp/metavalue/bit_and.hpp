// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/lift_value.hpp"

namespace mcl::mp {

/// Bitwise and of metavalues Vs
template<class... Vs>
using bit_and = lift_value<(Vs::value & ...)>;

/// Bitwise and of metavalues Vs
template<class... Vs>
constexpr auto bit_and_v = (Vs::value & ...);

}  // namespace mcl::mp
