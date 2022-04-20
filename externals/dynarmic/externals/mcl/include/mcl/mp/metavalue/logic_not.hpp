// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/value.hpp"

namespace mcl::mp {

/// Logical negation of metavalue V.
template<class V>
using logic_not = bool_value<!bool(V::value)>;

/// Logical negation of metavalue V.
template<class V>
constexpr bool logic_not_v = !bool(V::value);

}  // namespace mcl::mp
