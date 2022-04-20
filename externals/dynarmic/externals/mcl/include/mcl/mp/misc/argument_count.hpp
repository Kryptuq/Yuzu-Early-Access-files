// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/lift_value.hpp"

namespace mcl::mp {

/// Metafunction that returns the number of arguments it has
template<typename... Ts>
using argument_count = lift_value<sizeof...(Ts)>;

/// Metafunction that returns the number of arguments it has
template<typename... Ts>
constexpr auto argument_count_v = sizeof...(Ts);

}  // namespace mcl::mp
