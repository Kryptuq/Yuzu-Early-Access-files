// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metafunction/apply.hpp"
#include "mcl/mp/misc/argument_count.hpp"

namespace mcl::mp {

/// Length of list L
template<class L>
using length = apply<argument_count, L>;

/// Length of list L
template<class L>
constexpr auto length_v = length<L>::value;

}  // namespace mcl::mp
