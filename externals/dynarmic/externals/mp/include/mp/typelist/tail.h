/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace mp {

namespace detail {

template<class L>
struct tail_impl;

template<template<class...> class LT, class E1, class... Es>
struct tail_impl<LT<E1, Es...>> {
    using type = LT<Es...>;
};

} // namespace detail

/// Gets the first type of list L
template<class L>
using tail = typename detail::tail_impl<L>::type;

} // namespace mp
