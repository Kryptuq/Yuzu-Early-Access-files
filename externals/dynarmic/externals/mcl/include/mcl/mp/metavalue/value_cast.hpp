// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

namespace mcl::mp {

/// Casts a metavalue from one type to another
template<class T, class V>
using value_cast = std::integral_constant<T, static_cast<T>(V::value)>;

}  // namespace mcl::mp
