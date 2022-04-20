// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/value.hpp"

namespace mcl::mp {

/// Logical disjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
using logic_or = bool_value<(false || ... || Vs::value)>;

/// Logical disjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
constexpr bool logic_or_v = (false || ... || Vs::value);

}  // namespace mcl::mp
