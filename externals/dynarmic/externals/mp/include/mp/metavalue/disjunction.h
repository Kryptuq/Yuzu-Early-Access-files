/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/value.h>
#include <mp/metavalue/logic_if.h>

namespace mp {

namespace detail {

template<class...>
struct disjunction_impl;

template<>
struct disjunction_impl<> {
    using type = false_type;
};

template<class V>
struct disjunction_impl<V> {
    using type = V;
};

template<class V1, class... Vs>
struct disjunction_impl<V1, Vs...> {
    using type = logic_if<V1, V1, typename disjunction_impl<Vs...>::type>;
};

} // namespace detail

/// Disjunction of metavalues Vs with short-circuiting and type preservation.
template<class... Vs>
using disjunction = typename detail::disjunction_impl<Vs...>::type;

/// Disjunction of metavalues Vs with short-circuiting and type preservation.
template<class... Vs>
constexpr auto disjunction_v = disjunction<Vs...>::value;

} // namespace mp
