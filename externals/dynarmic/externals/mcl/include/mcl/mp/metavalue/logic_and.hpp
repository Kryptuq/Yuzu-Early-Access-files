// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/value.hpp"

namespace mcl::mp {

/// Logical conjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
using logic_and = bool_value<(true && ... && Vs::value)>;

/// Logical conjunction of metavalues Vs without short-circuiting or type presevation.
template<class... Vs>
constexpr bool logic_and_v = (true && ... && Vs::value);

}  // namespace mcl::mp
