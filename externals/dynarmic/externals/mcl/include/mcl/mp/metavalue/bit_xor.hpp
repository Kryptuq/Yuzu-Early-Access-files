// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/lift_value.hpp"

namespace mcl::mp {

/// Bitwise xor of metavalues Vs
template<class... Vs>
using bit_xor = lift_value<(Vs::value ^ ...)>;

/// Bitwise xor of metavalues Vs
template<class... Vs>
constexpr auto bit_xor_v = (Vs::value ^ ...);

}  // namespace mcl::mp
