// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <type_traits>

namespace mcl::mp {

/// A metavalue (of type VT and value v).
template<class VT, VT v>
using value = std::integral_constant<VT, v>;

/// A metavalue of type size_t (and value v).
template<size_t v>
using size_value = value<size_t, v>;

/// A metavalue of type bool (and value v). (Aliases to std::bool_constant.)
template<bool v>
using bool_value = value<bool, v>;

/// true metavalue (Aliases to std::true_type).
using true_type = bool_value<true>;

/// false metavalue (Aliases to std::false_type).
using false_type = bool_value<false>;

}  // namespace mcl::mp
