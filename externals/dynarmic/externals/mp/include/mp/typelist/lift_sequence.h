/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <type_traits>

#include <mp/typelist/list.h>

namespace mp {

namespace detail {

template<class VL>
struct lift_sequence_impl;

template<class T, template <class, T...> class VLT, T... values>
struct lift_sequence_impl<VLT<T, values...>> {
    using type = list<std::integral_constant<T, values>...>;
};

} // namespace detail

/// Lifts values in value list VL to create a type list.
template<class VL>
using lift_sequence = typename detail::lift_sequence_impl<VL>::type;

} // namespace mp
