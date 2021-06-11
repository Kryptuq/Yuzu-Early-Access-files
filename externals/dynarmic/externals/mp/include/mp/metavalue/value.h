/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstddef>
#include <type_traits>

namespace mp {

/// A metavalue (of type VT and value v).
template<class VT, VT v>
using value = std::integral_constant<VT, v>;

/// A metavalue of type std::size_t (and value v).
template<std::size_t v>
using size_value = value<std::size_t, v>;

/// A metavalue of type bool (and value v). (Aliases to std::bool_constant.)
template<bool v>
using bool_value = value<bool, v>;

/// true metavalue (Aliases to std::true_type).
using true_type = bool_value<true>;

/// false metavalue (Aliases to std::false_type).
using false_type = bool_value<false>;

} // namespace mp
