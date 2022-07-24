// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

namespace mcl::mp {

/// Do two metavalues contain the same value?
template<class V1, class V2>
using value_equal = std::bool_constant<V1::value == V2::value>;

}  // namespace mcl::mp
