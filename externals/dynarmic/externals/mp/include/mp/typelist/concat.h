/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/typelist/list.h>

namespace mp {

namespace detail {

template<class... Ls>
struct concat_impl;

template<>
struct concat_impl<> {
    using type = list<>;
};

template<class L>
struct concat_impl<L> {
    using type = L;
};

template<template<class...> class LT, class... E1s, class... E2s, class... Ls>
struct concat_impl<LT<E1s...>, LT<E2s...>, Ls...> {
    using type = typename concat_impl<LT<E1s..., E2s...>, Ls...>::type;
};

template<template<class...> class LT,
         class... E1s, class... E2s, class... E3s, class... E4s, class... E5s, class... E6s, class... E7s, class... E8s,
         class... E9s, class... E10s, class... E11s, class... E12s, class... E13s, class... E14s, class... E15s, class... E16s,
         class... Ls>
struct concat_impl<
        LT<E1s...>, LT<E2s...>, LT<E3s...>, LT<E4s...>, LT<E5s...>, LT<E6s...>, LT<E7s...>, LT<E8s...>,
        LT<E9s...>, LT<E10s...>, LT<E11s...>, LT<E12s...>, LT<E13s...>, LT<E14s...>, LT<E15s...>, LT<E16s...>,
        Ls...>
{
    using type = typename concat_impl<
        LT<
            E1s..., E2s..., E3s..., E4s..., E5s..., E6s..., E7s..., E8s...,
            E9s..., E10s..., E11s..., E12s..., E13s..., E14s..., E15s..., E16s...
        >,
        Ls...
    >::type;
};

} // namespace detail

/// Concatenate lists together
template<class... Ls>
using concat = typename detail::concat_impl<Ls...>::type;

} // namespace mp
