// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

namespace mcl::mp {

/// Lifts a value into a type (a metavalue)
template<auto V>
using lift_value = std::integral_constant<decltype(V), V>;

}  // namespace mcl::mp
