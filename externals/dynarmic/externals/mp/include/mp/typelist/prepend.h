/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace mp {

namespace detail {

template<class... L>
struct prepend_impl;

template<template<class...> class LT, class... E1s, class... E2s>
struct prepend_impl<LT<E1s...>, E2s...> {
    using type = LT<E2s..., E1s...>;
};

} // namespace detail

/// Prepend items E to list L
template<class L, class... Es>
using prepend = typename detail::prepend_impl<L, Es...>::type;

} // namespace mp
