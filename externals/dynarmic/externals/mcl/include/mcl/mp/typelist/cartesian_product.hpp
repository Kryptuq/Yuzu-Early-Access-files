// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metafunction/bind.hpp"
#include "mcl/mp/metafunction/map.hpp"
#include "mcl/mp/typelist/append.hpp"
#include "mcl/mp/typelist/concat.hpp"
#include "mcl/mp/typelist/list.hpp"

namespace mcl::mp {

namespace detail {

template<class... Ls>
struct cartesian_product_impl;

template<class RL>
struct cartesian_product_impl<RL> {
    using type = RL;
};

template<template<class...> class LT, class... REs, class... E2s>
struct cartesian_product_impl<LT<REs...>, LT<E2s...>> {
    using type = concat<
        map<MCL_MP_BIND(append, REs), list<E2s...>>...>;
};

template<class RL, class L2, class L3, class... Ls>
struct cartesian_product_impl<RL, L2, L3, Ls...> {
    using type = typename cartesian_product_impl<
        typename cartesian_product_impl<RL, L2>::type,
        L3,
        Ls...>::type;
};

}  // namespace detail

/// Produces the cartesian product of a set of lists
/// For example:
/// cartesian_product<list<A, B>, list<D, E>> == list<list<A, D>, list<A, E>, list<B, D>, list<B, E>
template<typename L1, typename... Ls>
using cartesian_product = typename detail::cartesian_product_impl<map<list, L1>, Ls...>::type;

}  // namespace mcl::mp
