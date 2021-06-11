/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace mp {

namespace detail {

template<class L>
struct head_impl;

template<template<class...> class LT, class E1, class... Es>
struct head_impl<LT<E1, Es...>> {
    using type = E1;
};

} // namespace detail

/// Gets the tail/cdr/all-but-the-first-element of list L
template<class L>
using head = typename detail::head_impl<L>::type;

} // namespace mp
