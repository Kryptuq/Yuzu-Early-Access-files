/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace mp {

namespace detail {

template<template<class...> class F, class L>
struct map_impl;

template<template<class...> class F, template<class...> class LT, class... Es>
struct map_impl<F, LT<Es...>> {
    using type = LT<F<Es>...>;
};

} // namespace detail

/// Applies each element of list L to metafunction F
template<template<class...> class F, class L>
using map = typename detail::map_impl<F, L>::type;

} // namespace mp
