// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/lift_value.hpp"

namespace mcl::mp {

/// Bitwise not of metavalue V
template<class V>
using bit_not = lift_value<~V::value>;

/// Bitwise not of metavalue V
template<class V>
constexpr auto bit_not_v = ~V::value;

}  // namespace mcl::mp
