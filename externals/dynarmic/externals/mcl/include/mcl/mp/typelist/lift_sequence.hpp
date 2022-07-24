// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

#include "mcl/mp/typelist/list.hpp"

namespace mcl::mp {

namespace detail {

template<class VL>
struct lift_sequence_impl;

template<class T, template<class, T...> class VLT, T... values>
struct lift_sequence_impl<VLT<T, values...>> {
    using type = list<std::integral_constant<T, values>...>;
};

}  // namespace detail

/// Lifts values in value list VL to create a type list.
template<class VL>
using lift_sequence = typename detail::lift_sequence_impl<VL>::type;

}  // namespace mcl::mp
