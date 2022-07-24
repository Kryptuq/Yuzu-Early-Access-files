// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <tuple>

#include "mcl/mp/metafunction/apply.hpp"

namespace mcl::mp {

/// Get element I from list L
template<std::size_t I, class L>
using get = std::tuple_element_t<I, apply<std::tuple, L>>;

}  // namespace mcl::mp
